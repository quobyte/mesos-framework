Quobyte Apache Mesos Framework
==============================

This Apache Mesos framework deploys the Quobyte Storage System on an Apache Mesos cluster.

Quobyte requires 3 different services to run and offers two additional Services:

* Registry Services: system Configuration; registry of services, devices, volumes
* Metadata Service: file metadata and locations
* Data Services: store file data
* API Service: Provides RPC API service for third party software
* Webconsole: User interface for monitoring and managing the Quobyte system


Dependencies:
=============
* mesos-dns. Keep your mesos subdomain at hand for further setup.
* a Docker registry with the Quobyte services container. You'll need its URL.


Usage
======

Get Quobyte
-----------

Familiarize yourself with Quobyte from the documentation. Get qmkdev and qbootstrap from the server package. Import the Quobyte docker images into your registry.

Setup Devices
-------------

Storage devices (hard drives, SSDs) are currently managed outside of Mesos. 
For each device that you want to purpose for Quobyte, install it, format it and mount it in a sub-directory of /mnt. 
Pick one device that will run the registry initially and run *qbootstrap* (part of Quobyte).
For all other devices, run *qmkdev* to tag it as a registry, metadata or data device.

Configure Framework
-------------------

The framework is currently configured via command line flags. The following flags need to be used normally:
* *--zk*: the Zookeeper URL
* *--master*: the mesos master, usually as a Zookeeper URL
* *--port*: port of the framework's built-in HTTP server with a console and API (default 7888).
* *--docker_image*: the name of the docker image (without version): [registry:port|name]/image
* *--mesos_dns_domain*: the subdomain where mesos-dns SRV records can be found. No leading dot.
* *--registry_dns_name*: manually set the registry hosts, format: host:rpcport[,host:rpcport]. rpcport is usually 21000.
* *--restrict_hosts*: comma-separated list of full mesos-agent hostnames
* *--port_range_base*: the port range to use for service ports (10 ports will be allocated). Default is 21000.
* *--api_port*: the port for the JSON-RPC API. You can reach the API on http://api.quobyte.slave.mesos:<portno>. Default is 8889.
* *--webconsole_port*: the port of the Quobyte Webconsole. You can reach it on http://webconsole.quobyte.slave.mesos:<portno>. Default is 8888.

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
* Use dynamic port assignments, when Mesos knows how to co-allocate tcp and dns ports.

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
