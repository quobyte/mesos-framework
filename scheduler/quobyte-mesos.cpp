/**
 * Copyright 2015 Quobyte Inc. All rights reserved.
 *
 * See LICENSE file for license details.
 */

#include <string>
#include <regex>
#include <functional>

#include <gflags/gflags.h>
#include <mesos/resources.hpp>
#include <mesos/scheduler.hpp>

#include "scheduler.hpp"
#include "http_server.hpp"

#include "state/zookeeper.hpp"
#include "state/state.hpp"

DEFINE_string(master, "",
              "Mesos master host:port or zk://host:port/mesos URL");
DEFINE_string(zk, "", "Zookeeper host:port");
DEFINE_string(zk_path, "/quobyte", "Zookeeper persistent state location");
DEFINE_int32(port, 7888, "Scheduler status port and API");
DEFINE_int32(failover_timeout_s, 24 * 3600, "Mesos framework timeout");
DEFINE_string(deployment, "default", "Quobyte deployment name");
DEFINE_string(framework_url, "", "URL of this framework, auto-generated if empty");
DEFINE_string(framework_name, "quobyte", "Mesos framework name");
DEFINE_string(framework_user, "", "Mesos framework user");
DEFINE_string(framework_role, "*", "Mesos framework role");
DEFINE_string(framework_principal, "*",
              "Mesos framework principal. "
              "Provide secret via environment variable QUOBYTE_MESOS_SECRET");
DEFINE_bool(reset, false, "Reset all state for this deployment");

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

static bool NotEmpty(const char* flagname, const std::string& value) {
  if (value.empty()) {
    printf("Please set flag %s\n", flagname);
    return false;
  }
  return true;
}

static const bool validate_master =
    gflags::RegisterFlagValidator(&FLAGS_master, &NotEmpty);
static const bool validate_zk =
    gflags::RegisterFlagValidator(&FLAGS_zk, &NotEmpty);

static std::string GetHostname() {
  char hostname[1024];
  hostname[1023] = '\0';
  gethostname(hostname, 1023);
  struct hostent* h;
  h = gethostbyname(hostname);
  return h->h_name;
}

int main(int argc, char* argv[]) {
  gflags::SetUsageMessage("Quobyte Mesos framework");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // TODO(felix): make most ports free-floating.
  SystemConfig systemConfig;
  systemConfig.registry.cpu = CPUS_PER_REGISTRY;
  systemConfig.registry.memory_mb = MEM_PER_REGISTRY;
  systemConfig.registry.http_port = REGISTRY_HTTP_PORT;
  systemConfig.registry.rpc_port = REGISTRY_RPC_PORT;

  systemConfig.metadata.cpu = CPUS_PER_METADATA;
  systemConfig.metadata.memory_mb = MEM_PER_METADATA;
  systemConfig.metadata.http_port = REGISTRY_HTTP_PORT + 10;
  systemConfig.metadata.rpc_port = REGISTRY_RPC_PORT + 10;

  systemConfig.data.cpu = CPUS_PER_DATA;
  systemConfig.data.memory_mb = MEM_PER_DATA;
  systemConfig.data.http_port = REGISTRY_HTTP_PORT + 20;
  systemConfig.data.rpc_port = REGISTRY_RPC_PORT + 20;

  systemConfig.api.cpu = CPUS_PER_API;
  systemConfig.api.memory_mb = MEM_PER_API;
  systemConfig.api.http_port = REGISTRY_HTTP_PORT + 30;
  systemConfig.api.rpc_port = REGISTRY_RPC_PORT + 30;

  systemConfig.webconsole.cpu = CPUS_PER_WEBCONSOLE;
  systemConfig.webconsole.memory_mb = MEM_PER_WEBCONSOLE;
  systemConfig.webconsole.http_port = REGISTRY_HTTP_PORT + 40;
  systemConfig.webconsole.rpc_port = REGISTRY_RPC_PORT + 40;

  mesos::internal::state::ZooKeeperStorage storage(
      FLAGS_zk, Seconds(120), FLAGS_zk_path);
  mesos::internal::state::State state_storage(&storage);

  const std::string state_path = "scheduler-" + FLAGS_deployment;
  SchedulerStateProxy state_proxy(&state_storage, state_path);

  if (FLAGS_reset) {
    LOG(INFO) << "Erased state for deployment " << FLAGS_deployment;
    state_proxy.erase();
    return 0;
  }

  const std::string framework_id = state_proxy.framework_id();
  if (framework_id.empty()) {
    LOG(INFO) << "No framework id found in " << state_path
        << ", starting as new framework";
  } else {
    LOG(INFO) << "Starting as framework " << framework_id;
  }

  mesos::FrameworkInfo framework;
  framework.set_user(FLAGS_framework_user);
  framework.set_name(FLAGS_framework_name);
  framework.set_role(FLAGS_framework_role);
  framework.set_principal(FLAGS_framework_principal);
  framework.set_failover_timeout(FLAGS_failover_timeout_s);
  framework.mutable_id()->set_value(framework_id);
  if (FLAGS_framework_url.empty()) {
    framework.set_webui_url(
        std::string("http://") + GetHostname() + ":" +
        std::to_string(FLAGS_port));
  } else {
    framework.set_webui_url(FLAGS_framework_url);
  }
  framework.set_checkpoint(true);

  QuobyteScheduler dfsScheduler(&state_proxy, &framework, systemConfig);

  quobyte::HttpServer http(FLAGS_port);
  http.Start(std::bind(&QuobyteScheduler::handleHTTP, &dfsScheduler,_1, _2, _3));
  LOG(INFO) << "Started http://" << GetHostname() << ":" << FLAGS_port;

  std::unique_ptr<mesos::MesosSchedulerDriver> schedulerDriver;

  const char* mesos_secret = getenv("QUOBYTE_MESOS_SECRET");
  if (mesos_secret != NULL) {
    LOG_IF(FATAL, FLAGS_framework_principal.empty())
        << "Authentication requires --framework_principal and QUOBYTE_MESOS_SECRET";
    mesos::Credential credential;
    credential.set_principal(FLAGS_framework_principal);
    credential.set_secret(mesos_secret);
    schedulerDriver.reset(
        new mesos::MesosSchedulerDriver(
            &dfsScheduler, framework, FLAGS_master, credential));
  } else {
    schedulerDriver.reset(
        new mesos::MesosSchedulerDriver(
            &dfsScheduler, framework, FLAGS_master));
  }

  const int status = schedulerDriver->run() == mesos::DRIVER_STOPPED ? 0 : 1;

  // Ensure that the driver process terminates.
  schedulerDriver->stop();

  http.Stop();

  return status;
}
