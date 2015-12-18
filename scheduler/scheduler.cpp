/**
 * Copyright 2015 Quobyte Inc. All rights reserved.
 *
 * See LICENSE file for license details.
 */

#include "scheduler.hpp"

#include <string>
#include <cstdint>
#include <chrono>
#include <google/protobuf/text_format.h>
#include <gflags/gflags.h>

#include <mesos/resources.hpp>
#include <mesos/scheduler.hpp>

const std::string PROBER_TASK = "prober";
const std::string REGISTRY_TASK = "registry";
const std::string METADATA_TASK = "metadata";
const std::string DATA_TASK = "data";
const std::string API_TASK = "api";
const std::string WEBCONSOLE_TASK = "webconsole";

DEFINE_int32(probe_interval_s, 60,
             "Device probe interval");
DEFINE_int32(probe_executor_keepalive_interval_s, 60,
             "Device probe executor keep-alive interval");
DEFINE_int32(reconcile_service_interval_s, 60,
             "Reconcile service at least every n seconds");
DEFINE_string(restrict_hosts, "",
              "Restrict scheduler to these hosts");
DEFINE_string(docker_image, "dockerregistry.corp.quobyte.com:5000/quobyte-service",
              "The docker image to run");
DEFINE_string(registry_dns_name, "_registry._tcp.quobyte.slave",
              "Quobyte registry configuration");
DEFINE_string(mesos_dns_domain, ".mesos",
              "Mesos DNS domain");
DEFINE_string(registry_resources, "cpus:1.0;mem:2084;disk:32",
              "Resources for registry");
DEFINE_string(metadata_resources, "cpus:2.0;mem:8192;disk:32",
              "Resources for metadata");
DEFINE_string(data_resources, "cpus:4.0;mem:4096;disk:32",
              "Resources for metadata");
DEFINE_string(prober_resources, "cpus:0.1;mem:256;disk:32",
              "Resources for prober");
DEFINE_string(api_resources, "cpus:0.1;mem:512;disk:32",
              "Resources for api");
DEFINE_string(webconsole_resources, "cpus:0.5;mem:512;disk:32",
              "Resources for webconsole");
DEFINE_string(extra_service_config, "",
              "Extra config for all service.cfg files");
DEFINE_string(host_device_directory,"/mnt",
              "Host directory where Quobyte devices are mounted");
DEFINE_int32(port_range_base, 21000,
             "Where to start allocating ports (10 required currently). This very port will be used for the registry RPC port.");
DEFINE_int32(api_port, 8889, "JSON-RPC API port");
DEFINE_int32(webconsole_port, 8888, "Webconsole HTTP port");

static const char* kExecutorId = "quobyte-mesos-prober-";
static const char* kArchiveUrl = "/executor.tar.gz";
static const char* kVersionAPIUrl = "/v1/version";
static const char* kHealthUrl = "/v1/health";
static const char* kDockerImageVersion = "docker_image_version";

static int64_t now() {
  std::chrono::steady_clock::time_point p = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(p.time_since_epoch()).count();
}

static bool IsTerminal(mesos::TaskState state) {
  switch (state) {
    case mesos::TASK_STAGING:
    case mesos::TASK_STARTING:
    case mesos::TASK_RUNNING:
      return false;
    case mesos::TASK_FINISHED:
    case mesos::TASK_FAILED:
    case mesos::TASK_KILLED:
    case mesos::TASK_LOST:
    case mesos::TASK_ERROR:
      return true;
  }
}

static bool DoStartService(quobyte::ServiceState_TaskState state) {
  switch (state) {
    case quobyte::ServiceState::STARTING:
    case quobyte::ServiceState::RUNNING:
    case quobyte::ServiceState::UNKNOWN:
      return false;
    case quobyte::ServiceState::NOT_RUNNING:
      return true;
  }
}

static void KillServiceIfRunning(
    mesos::SchedulerDriver* driver,
    const std::string& task_id_value,
    const quobyte::ServiceState& service) {
  if (service.state() == quobyte::ServiceState::RUNNING) {
    LOG(INFO) << "Shutting down " << task_id_value;
    mesos::TaskID task_id;
    task_id.set_value(task_id_value);
    driver->killTask(task_id);
  }
}

SchedulerStateProxy::SchedulerStateProxy(
    mesos::internal::state::State* state,
    const std::string& path) : state_(state), path_(path) {
  std::string textformat = state_->fetch(path_).get().value();
  google::protobuf::TextFormat::Parser p;
  if (!p.ParseFromString(textformat, &data_)) {
    LOG(FATAL) << "Could not parse " << textformat;
  }
  LOG(INFO) << "Initial framework state: " << data_.ShortDebugString();
}

std::string SchedulerStateProxy::framework_id() {
  return data_.framework_id_value();
}

void SchedulerStateProxy::set_framework_id(const std::string& id) {
  data_.set_framework_id_value(id);
  writeback();
}

void SchedulerStateProxy::set_target_version(const std::string& version) {
  data_.set_target_version(version);
  writeback();
}

const quobyte::SchedulerState& SchedulerStateProxy::state() {
  return data_;
}


void SchedulerStateProxy::writeback() {
  std::string serialized;
  google::protobuf::TextFormat::Printer p;
  if (!p.PrintToString(data_, &serialized)) {
    LOG(FATAL) << "Could not serialize " << data_.ShortDebugString();
  }

  mesos::internal::state::Variable variable =
      state_->fetch(path_).get();
  variable = variable.mutate(serialized);
  state_->store(variable);
}

void SchedulerStateProxy::erase() {
  data_.Clear();
  writeback();
}


QuobyteScheduler::QuobyteScheduler(
    SchedulerStateProxy* state,
    mesos::FrameworkInfo* framework)
    : state_(state),
      framework_(framework) {
  prepareServiceResources(
      REGISTRY_TASK,
      FLAGS_port_range_base,
      FLAGS_port_range_base + 1,
      FLAGS_registry_resources);

  prepareServiceResources(
      METADATA_TASK,
      FLAGS_port_range_base + 2,
      FLAGS_port_range_base + 3,
      FLAGS_metadata_resources);

  prepareServiceResources(
      DATA_TASK,
      FLAGS_port_range_base + 4,
      FLAGS_port_range_base + 5,
      FLAGS_data_resources);

  prepareServiceResources(
      API_TASK,
      FLAGS_port_range_base + 6,
      FLAGS_port_range_base + 7,
      FLAGS_api_resources);

  prepareServiceResources(
      WEBCONSOLE_TASK,
      FLAGS_port_range_base + 8,
      FLAGS_port_range_base + 9,
      FLAGS_webconsole_resources);

  mesos::Resources prober_resources =
      mesos::Resources::parse(
          FLAGS_prober_resources).get();
  resources_.emplace(PROBER_TASK, prober_resources);
}

void QuobyteScheduler::registered(mesos::SchedulerDriver* driver,
                                  const mesos::FrameworkID& framework_id,
                                  const mesos::MasterInfo&) {
  LOG(INFO) << "Storing framework id " << framework_id.value();
  state_->set_framework_id(framework_id.value());
  LOG(INFO) << "Quobyte Mesos framework registered. Reconciling.";
  std::vector<mesos::TaskStatus> status;
  driver->reconcileTasks(status);
}

void QuobyteScheduler::reregistered(mesos::SchedulerDriver* driver,
                                    const mesos::MasterInfo& masterInfo) {
  LOG(INFO) << "Quobyte Mesos framework re-registered. Reconciling.";
  std::vector<mesos::TaskStatus> status;
  driver->reconcileTasks(status);
}

void QuobyteScheduler::disconnected(mesos::SchedulerDriver* driver) {
  LOG(INFO) << "Ignoring disconnected.";
}

void QuobyteScheduler::slaveLost(mesos::SchedulerDriver* driver,
                                 const mesos::SlaveID& sid) {
  LOG(INFO) << "Ignoring slaveLost";
}

void QuobyteScheduler::error(mesos::SchedulerDriver* driver,
                             const std::string& message) {
  LOG(ERROR) << "Error " << message;
}


void QuobyteScheduler::createHost(const std::string& hostname,
   const std::string& slave_id) {
  quobyte::NodeState node_state;
  node_state.set_hostname(hostname);
  node_state.set_slave_id_value(slave_id);
  nodes_.insert(std::make_pair(hostname, node_state));
}


void QuobyteScheduler::reconcileHost(mesos::SchedulerDriver* driver,
                                     const mesos::Offer& offer) {
  createHost(offer.hostname(), offer.slave_id().value());

  std::vector<mesos::TaskStatus> status;
  mesos::TaskStatus prober;
  prober.set_state(mesos::TASK_LOST);
  prober.mutable_task_id()->set_value("quobyte-device-prober-" + offer.hostname());
  status.push_back(prober);
  prober.mutable_task_id()->set_value("quobyte-registry-" + offer.hostname());
  status.push_back(prober);
  prober.mutable_task_id()->set_value("quobyte-metadata-" + offer.hostname());
  status.push_back(prober);
  prober.mutable_task_id()->set_value("quobyte-data-" + offer.hostname());
  status.push_back(prober);
  prober.mutable_task_id()->set_value("quobyte-webconsole-" + offer.hostname());
  status.push_back(prober);
  prober.mutable_task_id()->set_value("quobyte-api-" + offer.hostname());
  status.push_back(prober);
  driver->reconcileTasks(status);
}

static bool NoRecentUpdates(const quobyte::ServiceState& service) {
  return service.last_update_s() == 0 ||
      now() - service.last_update_s() > FLAGS_reconcile_service_interval_s;
}

void QuobyteScheduler::resourceOffers(mesos::SchedulerDriver* driver,
                                      const std::vector<mesos::Offer>& offers) {
  std::vector<mesos::TaskInfo> tasks;

  for (const auto& offer : offers) {
    if (!FLAGS_restrict_hosts.empty() &&
        FLAGS_restrict_hosts.find(offer.hostname()) == -1) {
      driver->declineOffer(offer.id());
      VLOG(1) << "Ignoring host " << offer.hostname();
      continue;
    }

    VLOG(1) << "Offer for " << offer.hostname();
    std::map<std::string, quobyte::NodeState>::iterator node =
        nodes_.find(offer.hostname());
    if (node == nodes_.end()) {
      VLOG(1) << "New node " << offer.hostname();
      reconcileHost(driver, offer);
      driver->declineOffer(offer.id());
      continue;
    }

    mesos::Resources remaining_resources = offer.resources();
    quobyte::NodeState& node_state = node->second;
    if (now() - node_state.prober().last_seen_s() >
            FLAGS_probe_executor_keepalive_interval_s &&
        node_state.prober().state() != quobyte::ServiceState::RUNNING) {
      if (remaining_resources.contains(resources_[PROBER_TASK])) {
        node->second.mutable_prober()->set_state(quobyte::ServiceState::STARTING);
        node->second.mutable_prober()->set_last_update_s(now());
        LOG(INFO) << "Starting prober on " << offer.hostname();

        mesos::TaskInfo task;
        task.set_name("quobyte-device-prober");
        task.mutable_task_id()->set_value(
            "quobyte-device-prober-" + offer.hostname());
        task.mutable_slave_id()->MergeFrom(offer.slave_id());
        task.mutable_resources()->MergeFrom(resources_[PROBER_TASK]);

        mesos::CommandInfo command;
        mesos::CommandInfo::URI* uri = command.add_uris();
        uri->set_value(framework_->webui_url() + kArchiveUrl);
        uri->set_extract(true);
        uri->set_cache(false);
        command.set_user("felix");
        command.set_shell(true);
        command.set_value("./quobyte-mesos-executor");
        mesos::Environment* env = command.mutable_environment();
        mesos::Environment::Variable* var = env->add_variables();
        var->set_name("LD_LIBRARY_PATH");
        var->set_value(".");

        mesos::ExecutorInfo executor;
        executor.mutable_executor_id()->set_value(
            kExecutorId + state_->framework_id());
        executor.mutable_command()->CopyFrom(command);
        executor.set_source("quobyte");
        task.mutable_executor()->CopyFrom(executor);

        driver->launchTasks(offer.id(), std::vector<mesos::TaskInfo>({{task}}));
        continue;  // offer taken check next
      } else {
        LOG(ERROR) << "Not enough resources for prober on " << offer.hostname();
      }
    }

    if (node_state.prober().state() == quobyte::ServiceState::RUNNING &&
        now() - node_state.last_probe_s() > FLAGS_probe_interval_s) {
      VLOG(1) << "Triggering discovery on " << offer.hostname();
      // Trigger device discovery
      node_state.set_last_probe_s(now());
      mesos::ExecutorID executor_id;
      executor_id.set_value(
          kExecutorId + state_->framework_id());
      driver->sendFrameworkMessage(
          executor_id, offer.slave_id(),
          quobyte::ProbeRequest().SerializeAsString());
      // Also reconcile tasks
      reconcileHost(driver, offer);
      driver->declineOffer(offer.id());
      continue;
    }

    if (!state_->state().target_version().empty()) {
      std::vector<mesos::TaskInfo> tasks_to_start;
      if (remaining_resources.contains(resources_[API_TASK]) &&
          DoStartService(api_state_.state())) {
        remaining_resources -= resources_[API_TASK];
        tasks_to_start.push_back(
            makeTask(API_TASK,
                     "quobyte-api",
                     "quobyte-api-" + offer.hostname(),
                     offer.hostname(),
                     FLAGS_port_range_base + 6,
                     FLAGS_port_range_base + 7,
                     offer.slave_id()));
        api_state_.set_state(quobyte::ServiceState::RUNNING);
        api_state_.set_task_id("quobyte-api-" + offer.hostname());
      }
      if (remaining_resources.contains(resources_[WEBCONSOLE_TASK]) &&
          DoStartService(console_state_.state())) {
        remaining_resources -= resources_[WEBCONSOLE_TASK];
        tasks_to_start.push_back(
            makeTask(WEBCONSOLE_TASK,
                     "quobyte-webconsole",
                     "quobyte-webconsole-" + offer.hostname(),
                     offer.hostname(),
                     FLAGS_port_range_base + 8,
                     FLAGS_port_range_base + 9,
                     offer.slave_id()));
        console_state_.set_state(quobyte::ServiceState::RUNNING);
        console_state_.set_task_id("quobyte-webconsole-" + offer.hostname());
      }

      for (auto device_type : node_state.device_type()) {
        switch (device_type) {
          case quobyte::DeviceType::REGISTRY:
            if (DoStartService(node_state.registry().state())) {
              if (!remaining_resources.contains(resources_[REGISTRY_TASK])) {
                LOG(ERROR) << "Could not start registry: insufficient resources "
                    << offer.DebugString();
                node_state.mutable_registry()->set_last_message(
                    "Could not start registry: insufficient resources");
                continue;
              }
              LOG(INFO) << "Starting registry on " << offer.hostname();
              remaining_resources -= resources_[REGISTRY_TASK];

              tasks_to_start.push_back(
                  makeTask(REGISTRY_TASK,
                           "quobyte-registry",
                           "quobyte-registry-" + offer.hostname(),
                           offer.hostname(),
                           FLAGS_port_range_base,
                           FLAGS_port_range_base + 1,
                           offer.slave_id()));
            }
            break;
          case quobyte::DeviceType::METADATA:
            if (DoStartService(node_state.metadata().state())) {
              if (!remaining_resources.contains(resources_[METADATA_TASK])) {
                LOG(ERROR) << "Could not start metadata: insufficient resources "
                    << offer.DebugString();
                node_state.mutable_metadata()->set_last_message(
                    "Could not start metadata: insufficient resources");
                continue;
              }
              LOG(INFO) << "Starting metadata on " << offer.hostname();
              remaining_resources -= resources_[METADATA_TASK];

              tasks_to_start.push_back(
                  makeTask(METADATA_TASK,
                           "quobyte-metadata",
                           "quobyte-metadata-" + offer.hostname(),
                           offer.hostname(),
                           FLAGS_port_range_base + 2,
                           FLAGS_port_range_base + 3,
                           offer.slave_id()));
            }
            break;
          case quobyte::DeviceType::DATA:
            if (DoStartService(node_state.data().state())) {
              if (!remaining_resources.contains(resources_[DATA_TASK])) {
                LOG(ERROR) << "Could not start data: insufficient resources "
                    << offer.DebugString();
                node_state.mutable_data()->set_last_message(
                    "Could not start data: insufficient resources");
                continue;
              }

              LOG(INFO) << "Starting data on " << offer.hostname();
              remaining_resources -= resources_[DATA_TASK];

              tasks_to_start.push_back(
                  makeTask(DATA_TASK,
                           "quobyte-data",
                           "quobyte-data-" + offer.hostname(),
                           offer.hostname(),
                           FLAGS_port_range_base + 4,
                           FLAGS_port_range_base + 5,
                           offer.slave_id()));
            }
            break;
          default:
            break;
        }
      }
      if (!tasks_to_start.empty()) {
        driver->launchTasks(offer.id(), tasks_to_start);
        continue;
      }
    } else {
      KillServiceIfRunning(driver,
                           "quobyte-registry-" + offer.hostname(),
                           node_state.registry());
      KillServiceIfRunning(driver,
                           "quobyte-data-" + offer.hostname(),
                           node_state.data());
      KillServiceIfRunning(driver,
                           "quobyte-metadata-" + offer.hostname(),
                           node_state.metadata());
      KillServiceIfRunning(driver,
                           "quobyte-api-" + offer.hostname(),
                           api_state_);
      KillServiceIfRunning(driver,
                           "quobyte-webconsole-" + offer.hostname(),
                           console_state_);
    }
    driver->declineOffer(offer.id());
  }
}

void QuobyteScheduler::offerRescinded(mesos::SchedulerDriver* driver,
                                      const mesos::OfferID& offerId)  {
  LOG(INFO) << "Offer " << offerId.value() << " rescinded ";
}

quobyte::ServiceState* QuobyteScheduler::getService(
    quobyte::NodeState* node, const std::string& service) {
  if (service == "quobyte-device-prober") {
    return node->mutable_prober();
  } else if (service == "quobyte-registry") {
    return node->mutable_registry();
  } else if (service == "quobyte-metadata") {
    return node->mutable_metadata();
  } else if (service == "quobyte-data") {
    return node->mutable_data();
  } else if (service == "quobyte-api") {
    return &api_state_;
  } else if (service == "quobyte-webconsole") {
    return &console_state_;
  } else {
    return NULL;
  }
}

void QuobyteScheduler::statusUpdate(mesos::SchedulerDriver* driver,
                                    const mesos::TaskStatus& status)  {
  VLOG(1) << "statusUpdate " << status.ShortDebugString();
  const int pos = status.task_id().value().rfind("-");
  if (pos == -1) {
    return;
  }

  const std::string service = status.task_id().value().substr(0, pos);
  const std::string hostname = status.task_id().value().substr(pos + 1);

  std::map<std::string, quobyte::NodeState>::iterator node =
      nodes_.find(hostname);

  if (node == nodes_.end()) {
    VLOG(1) << "Node not found, creating " << hostname;
    createHost(hostname, status.slave_id().value());
    node = nodes_.find(hostname);
  }

  quobyte::ServiceState* service_state = getService(&node->second, service);
  if (service_state == NULL) {
    LOG(ERROR) << "Unknown service " << service;
    return;
  }

  if (status.state() == mesos::TASK_RUNNING ||
      status.state() == mesos::TASK_FINISHED) {
    bool service_should_run = true;

    if ((service == "quobyte-api" || service == "quobyte-webconsole") &&
        service_state->state() == quobyte::ServiceState::RUNNING &&
        service_state->task_id() != status.task_id().value()) {
      service_should_run = false;
    }

    if (node->second.device_types_valid()) {
      const std::set<int> device_types(
          node->second.device_type().begin(),
          node->second.device_type().end());
      if (service == "quobyte-registry" &&
          device_types.count(quobyte::DeviceType::REGISTRY) == 0) {
        service_should_run = false;
      } else if (service == "quobyte-metadata" &&
                 device_types.count(quobyte::DeviceType::METADATA) == 0) {
        service_should_run = false;
      } else if (service == "quobyte-data" &&
                 device_types.count(quobyte::DeviceType::DATA) == 0) {
        service_should_run = false;
      }
    }
    if (!service_should_run) {
      LOG(INFO) << "Service " << status.task_id().value()
            << " is no longer needed, killing";
      driver->killTask(status.task_id());
    } else if (status.task_id() == service_state->task_id() ||
               service_state->task_id().empty()) {
      service_state->set_state(quobyte::ServiceState::RUNNING);
      service_state->set_last_update_s(now());
      service_state->set_last_seen_s(now());
      service_state->set_last_message(status.message());
      service_state->set_task_id(status.task_id().value());
      LOG(INFO) << "Updated: " << status.task_id().value()
          << ": " << service_state->ShortDebugString();
    } else {
      VLOG(1) << "Ignoring info about running task " << status.task_id();
    }
  } else if (IsTerminal(status.state()) &&
             (status.task_id() == service_state->task_id() ||
              service_state->task_id().empty())) {
    service_state->set_state(quobyte::ServiceState::NOT_RUNNING);
    service_state->set_last_update_s(now());
    service_state->set_last_message(status.message());
    service_state->clear_task_id();
    LOG(INFO) << "Updated: " << status.task_id().value()
        << ": " << service_state->ShortDebugString();
  }

  if (service == "quobyte-device-prober" || IsTerminal(status.state())) {
    return;
  }

  // Do version checks for services
  std::string version;

  for (const mesos::Label& label : status.labels().labels()) {
    if (label.key() == kDockerImageVersion) {
      version = label.value();
      break;
    }
  }

  if (version.empty()) {
    VLOG(1) << "Did not find version in labels "
        << status.labels().ShortDebugString();
  }

  if (state_->state().target_version().empty()) {
    LOG(INFO) << "Shut down requested, killing " << status.task_id();
    driver->killTask(status.task_id());
  } else if (!version.empty() && version != state_->state().target_version()) {
    LOG(INFO) << "Version mismatch (target: "
        << state_->state().target_version() << ", actual: "
        << version << ")"
        ", restarting " << status.task_id();
  }
}

void QuobyteScheduler::frameworkMessage(mesos::SchedulerDriver* driver,
                                        const mesos::ExecutorID& executorId,
                                        const mesos::SlaveID& slaveId,
                                        const std::string& data)  {
  for (auto& node : nodes_) {
    if (node.second.slave_id_value() == slaveId.value()) {
      quobyte::ProbeResponse response;
      if (!response.ParseFromString(data)) {
        LOG(ERROR) << "Bad response";
        return;
      }
      LOG(INFO) << "Message from prober on " << slaveId.value()
          << " " << response.ShortDebugString();
      node.second.mutable_device_type()->CopyFrom(response.device_type());
      // We also know that the executor is alive
      node.second.mutable_prober()->set_last_seen_s(now());
      node.second.set_device_types_valid(true);
    }
  }
}

void QuobyteScheduler::executorLost(mesos::SchedulerDriver* driver,
                                    const mesos::ExecutorID& executorID,
                                    const mesos::SlaveID& slaveID,
                                    int status)  {
  LOG(ERROR) << "Lost executor " << executorID.ShortDebugString()
      << " on " << slaveID.ShortDebugString() << ": " << status;
}

static mesos::ContainerInfo createQbContainerInfo() {
  mesos::ContainerInfo containerInfo;

  containerInfo.set_type(mesos::ContainerInfo::DOCKER);
  mesos::Volume* devicesVol = containerInfo.add_volumes();
  // TODO(Silvan): make this configurable instead of hardcoded
  devicesVol->set_container_path("/devices");
  devicesVol->set_host_path(FLAGS_host_device_directory);
  devicesVol->set_mode(mesos::Volume::RW);

  return containerInfo;
}

mesos::ContainerInfo::DockerInfo QuobyteScheduler::createQbDockerInfo(
    const std::string& docker_image_version) {
  mesos::ContainerInfo::DockerInfo dockerInfo;

  dockerInfo.set_privileged(true);
  dockerInfo.set_network(mesos::ContainerInfo::DockerInfo::BRIDGE);
  dockerInfo.set_image(FLAGS_docker_image + ":" + docker_image_version);

  mesos::Parameter* param_i = dockerInfo.mutable_parameters()->Add();
  param_i->set_key("interactive");
  param_i->set_value("true");
  /*
  mesos::Parameter* param_t = dockerInfo.mutable_parameters()->Add();
  param_t->set_key("tty");
  param_t->set_value("true");
*/
  return dockerInfo;
}

static std::string constructDockerExecuteCommand(
    const std::string& service_name,
    const std::string& host_name,
    size_t rpcPort, size_t httpPort) {
  std::ostringstream rcs;

  rcs << "export QUOBYTE_SERVICE=" << service_name;
  rcs << " && export QUOBYTE_REGISTRY="
      << FLAGS_registry_dns_name << FLAGS_mesos_dns_domain;

  if (rpcPort != 0) {
    rcs << " && export QUOBYTE_RPC_PORT=" << rpcPort;
  } else {
    rcs << " && export QUOBYTE_RPC_PORT=$PORT0";
  }

  if (httpPort != 0) {
    rcs << " && export QUOBYTE_HTTP_PORT=" << httpPort;
  } else {
    rcs << " && export QUOBYTE_HTTP_PORT=$PORT1";
  }

  if (service_name == "api") {
    rcs << " && export QUOBYTE_API_PORT=" + std::to_string(FLAGS_api_port);
  }
  if (service_name == "webconsole") {
    rcs << " && export QUOBYTE_WEBCONSOLE_PORT=" + std::to_string(FLAGS_webconsole_port);
  }

  rcs << " && export HOST_IP=$(dig +short " + host_name + ")";
  if (!FLAGS_extra_service_config.empty()) {
    rcs << " && export QUOBYTE_EXTRA_SERVICE_CONFIG="
        << FLAGS_extra_service_config;
  }
  rcs << " && /opt/main.sh";

  VLOG(1) << "Constructed docker command: " << rcs.str() << "\n";

  return rcs.str();
}

static mesos::ContainerInfo::DockerInfo::PortMapping
    makePort(uint16_t port, const char* type) {
  mesos::ContainerInfo::DockerInfo::PortMapping result;
  result.set_host_port(port);
  result.set_container_port(port);
  result.set_protocol(type);
  return result;
}

static std::string buildResourceString(
    uint16_t rpcPort, uint16_t httpPort, uint16_t extraPort) {
  std::string result = "ports:[" +
          std::to_string(rpcPort) + "-" + std::to_string(rpcPort) +
          "," + std::to_string(httpPort) + "-" + std::to_string(httpPort);
  if (extraPort != 0) {
    result += "," + std::to_string(extraPort) + "-" + std::to_string(extraPort);
  }
  return result + "]";
}

void QuobyteScheduler::prepareServiceResources(
    const std::string& service_id,
    uint16_t rpcPort,
    uint16_t httpPort,
    const std::string& sys_resources) {
  std::string res_string = sys_resources;
  if (service_id == "api") {
    res_string += ";" + buildResourceString(rpcPort, httpPort, FLAGS_api_port);
  } else if (service_id == "webconsole") {
    res_string += ";" + buildResourceString(rpcPort, httpPort, FLAGS_webconsole_port);
  } else {
    res_string += ";" + buildResourceString(rpcPort, httpPort, 0);
  }

  LOG(INFO) << service_id << " " << res_string;
  mesos::Resources resources = mesos::Resources::parse(res_string).get();
  resources_.emplace(service_id, resources);
}

mesos::TaskInfo QuobyteScheduler::makeTask(const std::string& service_id,
                                           const std::string& name,
                                           const std::string& task_id,
                                           const std::string& host_name,
                                           uint16_t rpcPort,
                                           uint16_t httpPort,
                                           const mesos::SlaveID& slave_id) {
  // Not semantically equivalent, but works for now
  std::string systemd_service_name = service_id;

  mesos::TaskInfo taskInfo;
  taskInfo.set_name(name);
  taskInfo.mutable_task_id()->set_value(task_id);
  taskInfo.mutable_slave_id()->MergeFrom(slave_id);
  taskInfo.mutable_resources()->MergeFrom(resources_[service_id]);

  const std::string docker_image_version = state_->state().target_version();
  mesos::ContainerInfo containerInfo = createQbContainerInfo();
  mesos::ContainerInfo::DockerInfo dockerInfo =
      createQbDockerInfo(docker_image_version);

  // docker port std::mappings
  *dockerInfo.add_port_mappings() = makePort(rpcPort, "tcp");
  *dockerInfo.add_port_mappings() = makePort(rpcPort, "udp");
  *dockerInfo.add_port_mappings() = makePort(httpPort, "tcp");
  if (service_id == "api") {
    *dockerInfo.add_port_mappings() = makePort(FLAGS_api_port, "tcp");
  }
  if (service_id == "webconsole") {
    *dockerInfo.add_port_mappings() = makePort(FLAGS_webconsole_port, "tcp");
  }
  containerInfo.mutable_docker()->CopyFrom(dockerInfo);

  mesos::CommandInfo command;
  command.set_value(constructDockerExecuteCommand(systemd_service_name,
                                                  host_name,
                                                  rpcPort,
                                                  httpPort));
  command.set_shell(true);

  taskInfo.mutable_command()->CopyFrom(command);
  taskInfo.mutable_container()->CopyFrom(containerInfo);

  mesos::Label* version = taskInfo.mutable_labels()->add_labels();
  version->set_key(kDockerImageVersion);
  version->set_value(docker_image_version);

  mesos::DiscoveryInfo* discovery = taskInfo.mutable_discovery();
  discovery->set_visibility(mesos::DiscoveryInfo::FRAMEWORK);
  discovery->set_name(service_id);
  discovery->set_version(docker_image_version);

  LOG(INFO) << "Launching " << taskInfo.DebugString();

  return taskInfo;
}

std::string QuobyteScheduler::handleHTTP(
    const std::string& method,
    const std::string& path,
    const std::string& data) {
  LOG(INFO) << method << " request to " << path << " with " << data;
  if (method == "GET" && path == kArchiveUrl) {
    int fd = open("executor/executor.tar.gz", O_RDONLY);
    if (fd == -1) {
      LOG(ERROR) << "Could not open executor";
      return "";
    }
    const uint32_t kBufferSize = 10*1024*1024;
    std::unique_ptr<char[]> buffer(new char[kBufferSize]);
    int bytes = read(fd, buffer.get(), kBufferSize);
    if (bytes <=0 || bytes == kBufferSize) {
      LOG(FATAL) << "Binary too large " << bytes;
    }
    close(fd);
    return std::string(buffer.get(), bytes);
  } else if (path.find(kVersionAPIUrl) == 0) {
    LOG(INFO) << "Rolling out version " << data;
    state_->set_target_version(data);
    return state_->state().target_version();
  } else if (path.find(kHealthUrl) == 0) {
    LOG(INFO) << "Rolling out version " << data;
    return "OK";
  } else {
    std::string result;
    result = "<html><body><h1>Quobyte Mesos Executor</h1>\n";
    result += state_->state().DebugString() + "<br>";
    result += "<div style=\"display: block\"><h2>API</h2>" +
        api_state_.DebugString() + "</div>";
    result += "<div style=\"display: block\"><h2>Web Console</h2>" +
        console_state_.DebugString() + "</div>";

    for (const auto& node : nodes_) {
      result += "<div style=\"display: inline-block; border: 1px solid black\"><h2>" + 
          node.first + "</h2><pre>" +
          node.second.DebugString() + "</pre></div>\n";
    }

    result += "</body></html>";
    return result;
  }
}
