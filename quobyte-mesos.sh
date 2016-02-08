#!/bin/bash
echo "Zookeeper: $ZOOKEEPER"
echo "Mesos master: $MESOS_MASTER"
echo "HTTP port: $PORT"
echo "Docker image: $DOCKER_IMAGE"
echo "mesos-dns domain: $MESOS_DNS_DOMAIN"
echo "framework image: $FRAMEWORK_IMAGE"

export LD_LIBRARY_PATH=/opt:$LD_LIBRARY_PATH 
cd /opt
/opt/quobyte-mesos --zk=$ZOOKEEPER --master=$MESOS_MASTER --port=$PORT --docker_image=$DOCKER_IMAGE --mesos_dns_domain=$MESOS_DNS_DOMAIN --framework_image=$FRAMEWORK_IMAGE
