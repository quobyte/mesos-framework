Quobyte Framework for Apache Mesos / Mesosphere DCOS
====================================================

This Apache Mesos framework deploys the Quobyte Storage System on an Apache Mesos or Mesosphere DCOS cluster.

Quobyte solves the stateful container problem by providing containers with persistent storage that can be uniformely accessed no matter where the container is scheduled.

Quobyte is a fully fault-tolerant distributed POSIX file system. It aggregates the available storage resources of a cluster
and makes them available for highly-available and scalable file storage. It can run directly on hosts or in public clouds. In
public clouds, it works with the the respective persistent volume services instead of local drives.

As a storage platform for a container infrastructures Quobyte can support applications ranging from databases to Big Data setups. 
For more details see to http://www.quobyte.com.

Documentation
=============

1. [General usage documentation](https://github.com/quobyte/mesos-framework/blob/master/docs/usage.md) on how to manually set up and run the Quobyte Mesos framework in a Mesos cluster
1. A [high level architecture](https://github.com/quobyte/mesos-framework/blob/master/docs/fw_architecture.md) describing the basic Quobyte Mesos framework elements and interactions
1. A [step by step DC/OS tutorial](https://github.com/quobyte/mesos-framework/blob/master/docs/aws_tutorial.md) on how to deploy the Quobyte Mesos framework on a Mesosphere DC/OS cluster hosted on AWS
