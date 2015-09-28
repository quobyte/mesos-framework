/**
 * Copyright 2015 Quobyte Inc. All rights reserved.
 *
 * See LICENSE file for license details.
 */

#include "executor.hpp"

#include <set>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>

#include "quobyte.pb.h"

void QuobyteExecutor::registered(
    mesos::ExecutorDriver* driver,
    const mesos::ExecutorInfo& executorInfo,
    const mesos::FrameworkInfo& frameworkInfo,
    const mesos::SlaveInfo& slaveInfo) {
  std::cout << "registered" << std::endl;
}

void QuobyteExecutor::reregistered(
    mesos::ExecutorDriver* driver,
    const mesos::SlaveInfo& slaveInfo) {
  std::cout << "reregistered" << std::endl;
}

void QuobyteExecutor::disconnected(mesos::ExecutorDriver* driver) {
  std::cout << "disconnected" << std::endl;
}

void QuobyteExecutor::launchTask(
    mesos::ExecutorDriver* driver,
    const mesos::TaskInfo& task) {
  std::cout << "launchTask" << std::endl;

  mesos::TaskStatus status;
  status.mutable_task_id()->MergeFrom(task.task_id());
  status.set_state(mesos::TASK_FINISHED);
  driver->sendStatusUpdate(status);
}

void QuobyteExecutor::killTask(
    mesos::ExecutorDriver* driver,
    const mesos::TaskID& taskId) {
  std::cout << "killTask" << std::endl;
}

void QuobyteExecutor::frameworkMessage(
    mesos::ExecutorDriver* driver,
    const std::string& data) {
  std::cout << "Received probe devices request" << std::endl;
  std::set<quobyte::DeviceType> found_types;
  std::ifstream infile("/proc/mounts");
  std::string line;
  while (std::getline(infile, line)) {
    std::istringstream iss(line);
    std::string what, mntpoint, type;
    if (!(iss >> what >> mntpoint >> type)) {
      std::cerr << "Could not parse " << line;
      continue;
    }
    std::cout << "Investigating " << mntpoint << std::endl;

    if (type == "ext4" || type == "xfs") {
      for (const std::string name :
           std::vector<const char*>({{"dir"}, {"metadata"}, {"data"}})) {
        struct stat status;
        if (stat((mntpoint + "/quobyte-" + name).c_str(), &status) == 0) {
          if (name == "dir") {
            found_types.insert(quobyte::REGISTRY);
          } else if (name == "metadata") {
            found_types.insert(quobyte::METADATA);
          } else if (name == "data") {
            found_types.insert(quobyte::DATA);
          }
        }
      }
    }
  }

  quobyte::ProbeResponse response;
  for (auto type : found_types) {
    response.add_device_type(type);
  }

  if (found_types.empty()) {
    return;
  }

  std::cout << "Found device types: "
      << response.ShortDebugString() << std::endl;
  std::string result;
  if (!response.SerializeToString(&result)) {
    std::cout << "Could not serialize " << response.DebugString() << std::endl;
  }

  driver->sendFrameworkMessage(result);
}

void QuobyteExecutor::shutdown(mesos::ExecutorDriver* driver) {
  std::cout << "shutdown" << std::endl;
}

void QuobyteExecutor::error(
    mesos::ExecutorDriver* driver,
    const std::string& message) {
  std::cout << "error " << message << std::endl;
}
