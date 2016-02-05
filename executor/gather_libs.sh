#!/bin/bash
rm -Rf .libs
mkdir -p .libs
cp -vaL ../thirdparty/mesos/build/src/.libs/libmesos-$MESOS_VERSION.so .libs
cp -vaL /lib64/libdl.so.2 /lib64/ld-linux-x86-64.so.2 .libs

LIST=$(LD_LIBRARY_PATH=.libs ldd quobyte-mesos-executor | tail -n +2 | awk '{printf "%s ", $3}')
echo $LIST
cp -vaL $LIST .libs || true
