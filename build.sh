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

strip --strip-all thirdparty/mesos/build/src/.libs/libmesos-*.so

echo "Build framework"
cd ../../..
make clean
make
