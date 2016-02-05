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
                   mesos::FrameworkInfo* framework);
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
  mesos::TaskInfo makeTask(const std::string& service_id,
                           const std::string& name,
                           const std::string& task_id,
                           const std::string& host_name,
                           uint16_t rpcPort,
                           uint16_t httpPort,
                           const mesos::SlaveID& slave_id);

  void prepareServiceResources(
      const std::string& service_id,
      uint16_t rpcPort,
      uint16_t httpPort,
      const std::string& resources);

  quobyte::ServiceState* getService(
      quobyte::NodeState* node, const std::string& service);

  void reconcileHost(
      mesos::SchedulerDriver* driver,
      const mesos::Offer& offer);

  void createHost(const std::string& hostname,
      const std::string& slave_id);

  SchedulerStateProxy* state_;
  mesos::FrameworkInfo* framework_;

  std::map<std::string, mesos::Resources> resources_;

  quobyte::ServiceState api_state_;
  quobyte::ServiceState console_state_;
  std::map<std::string, quobyte::NodeState> nodes_;
};

