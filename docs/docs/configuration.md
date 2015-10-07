---
title: Configuration
---

# Configuration

For configuration options available with the DCOS CLI please refer to

    dcos quobyte -h

information output.

The Quobyte Mesos framework itself provides the following config parameters:

Zookeeper
- zk - Name or IP address of the zookeeper server
- master - Name, IP address or Zookeeper URL of the Mesos master server
- port - Port to which the framework's HTTP server is bound
- docker_image - Docker image to be used for the Quobyte services
- mesos-dns_domain - The Mesos DNS domain featuring SRV records in the Mesos cluster

