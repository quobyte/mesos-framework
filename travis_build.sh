#!/bin/bash
set -ev
export MESOS_VERSION=0.24.1
./build.sh
./push_docker.sh

export MESOS_VERSION=0.25
./build.sh
./push_docker.sh
