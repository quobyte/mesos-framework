/**
 * Copyright 2015 Quobyte Inc. All rights reserved.
 *
 * See LICENSE file for license details.
 */

#pragma once

#include <string>
#include <cstdint>

#include <mesos/resources.hpp>
#include <mesos/scheduler.hpp>
#include "state/state.hpp"

#include "quobyte.pb.h"
#include "config.hpp"

class SchedulerStateProxy {
 public:
  SchedulerStateProxy(mesos::internal::state::State* state,
                      const std::string& path);
  void erase();
  std::string framework_id();
  void set_framework_id(const std::string& id);

  void set_target_version(const std::string& version);

  const quobyte::SchedulerState& state();

 private:
  void writeback();

  mesos::internal::state::State* state_;
  const std::string path_;
  quobyte::SchedulerState data_;
};

class QuobyteScheduler : public mesos::Scheduler {
public:
  QuobyteScheduler(SchedulerStateProxy* state,
                   mesos::FrameworkInfo* framework,
                   const SystemConfig& systemConfig);
  virtual ~QuobyteScheduler() {}

  virtual void registered(mesos::SchedulerDriver* driver,
                          const mesos::FrameworkID&,
                          const mesos::MasterInfo&) override;
  virtual void reregistered(mesos::SchedulerDriver* driver,
                            const mesos::MasterInfo& masterInfo) override;
  virtual void disconnected(mesos::SchedulerDriver* driver) override;

  virtual void error(mesos::SchedulerDriver* driver,
                     const std::string& message) override;

  virtual void resourceOffers(mesos::SchedulerDriver* driver,
                              const std::vector<mesos::Offer>& offers) override;

  virtual void offerRescinded(mesos::SchedulerDriver* driver,
                              const mesos::OfferID& offerId) override;

  virtual void statusUpdate(mesos::SchedulerDriver* driver,
                            const mesos::TaskStatus& status) override;

  virtual void frameworkMessage(mesos::SchedulerDriver* driver,
                                const mesos::ExecutorID& executorId,
                                const mesos::SlaveID& slaveId,
                                const std::string& data) override;

  virtual void slaveLost(mesos::SchedulerDriver* driver,
                         const mesos::SlaveID& sid) override;

  virtual void executorLost(mesos::SchedulerDriver* driver,
                            const mesos::ExecutorID& executorID,
                            const mesos::SlaveID& slaveID,
                            int status) override;

  std::string handleHTTP(const std::string& method,
                         const std::string& path,
                         const std::string& data);

 private:
  static std::string buildResourceString(
      float cpus, size_t mem, uint16_t rpcPort, uint16_t httpPort);

  static mesos::ContainerInfo createQbContainerInfo(const std::string& devDir);

  static mesos::ContainerInfo::DockerInfo createQbDockerInfo(
      const std::string& docker_image_version);

  static std::string constructDockerExecuteCommand(
      const std::string& service, size_t rpcPort, size_t httpPort);

  static mesos::ContainerInfo::DockerInfo::PortMapping
      makePort(uint16_t port, const char* type);

  mesos::TaskInfo makeTask(const std::string& service_id,
                           const std::string& name,
                           const std::string& task_id,
                           uint16_t rpcPort,
                           uint16_t httpPort,
                           const mesos::SlaveID& slave_id);

  void prepareServiceResources(
      const std::string& service_id,
      uint16_t rpcPort,
      uint16_t httpPort,
      float cpu,
      uint32_t memory);

  quobyte::ServiceState* getService(
      quobyte::NodeState* node, const std::string& service);

  SystemConfig system_config_;
  SchedulerStateProxy* state_;
  mesos::FrameworkInfo* framework_;

  std::map<std::string, mesos::Resources> resources_;

  std::string device_directory;

  quobyte::ServiceState api_state_;
  quobyte::ServiceState console_state_;
  std::map<std::string, quobyte::NodeState> nodes_;
};
