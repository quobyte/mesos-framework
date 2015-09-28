Mesos Quobyte Framework
=======================

The Mesos Quobyte framework deploys the Quobyte Storage System on a Mesos cluster.

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

Build
-----
Checkout branch 0.23 of the mesos submodule, build (see Apache Mesos), run make.

Setup Devices
-------------

Storage devices (hard drives, SSDs) are currently managed outside of Mesos. 
For each device that you want to purpose for Quobyte, install it, format it and mount it. 
Pick one device that will run the registry initially and run *qbootstrap* (part of Quobyte).
Then run *qmkdev* to tag it as a registry, metadata or data device.


Configure Framework
-------------------

The framework is currently configured via command line flags. The following flags need to be used normally:
* *--zk*: the Zookeeper URL
* *--master*: the mesos master, usually as a Zookeeper URL
* *--port*: port of the framework's built-in HTTP server with a console and API
* *--docker_image*: the name of the docker image (without version): [registry:port|name]/image
* *--mesos_dns_domain*: the subdomain where mesos-dns SRV records can be found. No leading dot.

If something goes wrong, the *--reset* flag might be helpful, which deletes the framework's state on Zookeeper.

Run
---

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

Limitations/Features:
====================
Desirable improvements:
* Configurable resource requirements.
* Rolling updates on version changes.


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
