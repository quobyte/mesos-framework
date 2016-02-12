#!/bin/bash
export LD_LIBRARY_PATH=/opt:$LD_LIBRARY_PATH 
cd /opt
/opt/quobyte-mesos "$@"
