#!/bin/bash
echo "Build docker image"
sudo docker build -t quobyte/quobyte-mesos:latest .
sudo docker push quobyte/quobyte-mesos:latest
