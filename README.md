Quobyte Framework for Apache Mesos / Mesosphere DCOS
====================================================

This Apache Mesos framework deploys the Quobyte Storage System on an Apache Mesos or Mesosphere DCOS cluster.

Quobyte is a fully fault-tolerant distributed POSIX file system. It aggregates the available storage resources of a cluster
and makes them available for highly-available and scalable file storage. It can run directly on hosts or in public clouds. In
public clouds, it works with the the respective persistent volume services instead of local drives.

As a storage platform for a container infrastructures Quobyte can support applications ranging from databases to Big Data setups. 
For more details see to http://www.quobyte.com.

This framework deploys Quobyte on a Mesos cluster by starting the respective Quobyte services on each host that has storage resources.
Quobyte consists of three core services:
* Registry Services: registry of services, devices, volumes. Usually 3 replicas per deployment.
* Metadata Service: file metadata and file locations
* Data Services: file data

Two additional services provide access to the system's management interface:
* API Service: JSONRPC API service for third-party software
* Webconsole: user interface for monitoring and managing the Quobyte system

Dependencies:
=============
* mesos-dns. Keep your mesos subdomain at hand for further setup.
* a Docker registry with the Quobyte services container. You'll need its URL.

Usage
======

Preparations
------------

Quobyte is not yet available for direct download. However, you can get access to the Quobyte download and documentation portal by sending an email to info@quobyte.com.

Further:
* Familiarize yourself with Quobyte from the documentation. 
* Get qmkdev and qbootstrap from the quobyte-server package. 
* Load the Quobyte docker images into your registry as described [here](https://support.quobyte.com/docs/2/latest/container_setup.html)

Setup Storage Devices
---------------------

Quobyte Storage Devices are storage devices local to the host. This means:
* servers: local HDs and SSDs
* AWS: mounted EBS volumes
* GCE: mounted persistent volumes

Storage devices need to be formatted (ext4), tagged with qmkdev, and mounted in a sub-directory of /mnt.
Pick one device that will run the registry initially and run *qbootstrap*. You need at least one registry, one metadata and
one data device.

On AWS or GCE attach the devices across one or several instances.

Configure Framework
-------------------

The framework is currently configured via command line flags. The following flags need to be used normally:
* *--zk*: the Zookeeper URL
* *--master*: the mesos master, usually as a Zookeeper URL
* *--port*: port of the framework's built-in HTTP server with a console and API (default 7888).
* *--docker_image*: the name of the Docker quobyte-server image (without version): [registry:port|name]/image
* *--framework_image*: the name of the Docker quobyte-mesos image, usually quobyte/quobyte-mesos:latest 
* *--mesos_dns_domain*: the subdomain where mesos-dns SRV records can be found. No leading dot.
* *--registry_dns_name*: manually set the registry hosts, format: host:rpcport[,host:rpcport]. rpcport is usually 21000.
* *--restrict_hosts*: comma-separated list of full mesos-agent hostnames
* *--port_range_base*: the port range to use for service ports (10 ports will be allocated). Default is 21000.
* *--api_port*: the port for the JSON-RPC API. You can reach the API on http://api.quobyte.slave.mesos:<portno>. Default is 8889.
* *--webconsole_port*: the port of the Quobyte Webconsole. You can reach it on http://webconsole.quobyte.slave.mesos:<portno>. Default is 8888.
* *--framework_url*: the URL of the framework. Auto-generated if not set. You have to set it if you run the framework on changing hosts, like from Marathon, otherwise you get "invalid ExecutorInfo".

If something goes wrong, the *--reset* flag might be helpful, which deletes the framework's state on Zookeeper.

Make sure that --registry_dns_name is set to hosts on which you created registry devices.

Refer to the Quobyte documentation, specifically the Quick Start Guide on how to bootstrap registries.

Run on the command line
-----------------------

```
sudo docker run quobyte/quobyte-mesos:0.25.0 /opt/quobyte-mesos-cmd.sh --zk=zk.corp:2181 --master=zk://zk.corp:2181/mesos ...
```

Running the Framework on Marathon
---------------------------------

```
{
   "id":"/quobyte-mesos",
   "cmd":"/opt/quobyte-mesos-cmd.sh --zk=zk.corp:2181 --master=zk://zk.corp:2181/mesos --docker_image=docker-registry.corp:5000/quobyte-server --mesos_dns_domain=mesos",
   "instances":1,
   "cpus":0.1,
   "mem":256.0,
   "ports":[7888],
   "requirePorts": true,
   "container": {
    "type": "DOCKER",
    "docker": {
      "forcePullImage": true,
      "image": "quobyte/quobyte-mesos:0.25.0",
      "network": "HOST"
    }
  }
}
```

Control the framework
---------------------

Start the framework, then let it deploy a version with:
```
curl -X POST --data "1.1.5" 'http://<framework-host>:<port>/v1/version'
```

In order to shut it down:
```
curl -X POST --data "" 'http://<framework-host>:<port>/v1/version'
```

When a new version of Quobyte comes out, you can upgrade the cluster with:
```
curl -X POST --data "1.1.6" 'http://<framework-host>:<port>/v1/version'
```

Health Monitoring
-----------------

The framework exports /v1/health for health monitoring.


Uninstall
---------

You can shut down all Quobyte tasks with a POST to /v1/version (see above). The only state left on hosts
are any Quobyte devices and their content.

The framework has a flag --reset to erase its state from Zookeeper.


Troubleshooting
---------------

If you get **invalid ExecutorInfo** errors, you are running the framework on a new host. Please --reset it.


Build from source
=================

Build mesos (if in doubt refer to http://mesos.apache.org/gettingstarted/). Make sure that you do not have installed -devel packages of Mesos direct and indirect third party dependencies. Also make sure that you build for the Mesos version that you have deployed (which might be >0.24.1).
```
$ cd thirdparty/mesos; git checkout 0.23.0
$ ./bootstrap
$ mkdir build
$ cd build
$ ../configure
$ make
```

Build the Quobyte framework and its dependencies:
```
$ make
```

Or use the supplied build.sh



Limitations/Features:
====================
Desirable improvements:
* Does not use mesos-dns for registry discovery yet as mesos-dns does not support multiple ports. (See [issue #61](https://github.com/mesosphere/mesos-dns/issues/61))
* Rolling updates on version changes does not work yet, as Mesos does not export labels back to framework. (See [MESOS-4135](https://issues.apache.org/jira/browse/MESOS-4135))
* Use dynamic port assignments, when Mesos knows how to co-allocate tcp and dns ports. (See [MESOS-4485](https://issues.apache.org/jira/browse/MESOS-4485))

References:
==========

https://github.com/quobyte/quobyte-automation/wiki/Docker-Demo-Setup-on-localhost
https://github.com/quobyte/quobyte-automation/tree/master/docker


Dependencies
-------------
- [mesosphere/mesos](https://github.com/mesosphere/mesos)
- [google/protobuf](https://github.com/google/protobuf)
- [google/glog](https://github.com/google/glog)
- [google/gflags](https://github.com/google/gflags)
