/**
 * Copyright 2015 Quobyte Inc. All rights reserved.
 *
 * See LICENSE file for license details.
 */

#include "mesos/executor.hpp"

class QuobyteExecutor : public mesos::Executor {
 public:
  QuobyteExecutor() {}
  virtual ~QuobyteExecutor() {}

  virtual void registered(
      mesos::ExecutorDriver* driver,
      const mesos::ExecutorInfo& executorInfo,
      const mesos::FrameworkInfo& frameworkInfo,
      const mesos::SlaveInfo& slaveInfo);

  virtual void reregistered(
      mesos::ExecutorDriver* driver,
      const mesos::SlaveInfo& slaveInfo);

  virtual void disconnected(mesos::ExecutorDriver* driver);

  // callback has returned.
  virtual void launchTask(
      mesos::ExecutorDriver* driver,
      const mesos::TaskInfo& task);

  virtual void killTask(
      mesos::ExecutorDriver* driver,
      const mesos::TaskID& taskId);

  virtual void frameworkMessage(
      mesos::ExecutorDriver* driver,
      const std::string& data);

  virtual void shutdown(mesos::ExecutorDriver* driver);

  virtual void error(
      mesos::ExecutorDriver* driver,
      const std::string& message);
};
