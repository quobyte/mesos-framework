#!/bin/bash
set -ev
export MESOS_VERSION=0.25.0
./build.sh
./push_docker.sh
