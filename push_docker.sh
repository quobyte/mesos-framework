#!/bin/bash
echo "Build docker image"
sudo docker build -t quobyte/quobyte-mesos:$MESOS_VERSION .
sudo docker push quobyte/quobyte-mesos:$MESOS_VERSION
