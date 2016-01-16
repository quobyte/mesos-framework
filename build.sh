#!/bin/bash

echo "Build framework for Mesos $MESOS_VERSION"
git submodule update --init --recursive

cd thirdparty/mesos
git fetch --tags
git checkout $MESOS_VERSION
 ./bootstrap
mkdir build
cd build
../configure
make

echo "Build framework"
cd ../../..
make clean
make
