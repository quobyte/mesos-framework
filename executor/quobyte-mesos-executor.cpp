/**
 * Copyright 2015 Quobyte Inc. All rights reserved.
 *
 * See LICENSE file for license details.
 */

#include "executor.hpp"

int main(int argc, char* argv[]) {
  QuobyteExecutor executor;
  mesos::MesosExecutorDriver executor_driver(&executor);
  const int status = executor_driver.run() == mesos::DRIVER_STOPPED ? 0 : 1;

  // Ensure that the driver process terminates.
  executor_driver.stop();

  return status;
}
