
Quobyte Mesos Framework - Usage
===============================

Preparations
------------

You can get access to the Quobyte download and documentation portal by sending an email to info@quobyte.com.

Further:

* Familiarize yourself with Quobyte from the documentation. 
* Get qmkdev and qbootstrap from the quobyte-server package. 
* Load the Quobyte docker images into your registry as described [here](https://support.quobyte.com/docs/2/latest/container_setup.html)

Setup Storage Devices
---------------------

Quobyte Storage Devices are storage devices local to the host. Depending on bare metal or cloud setups they come in several forms:

* servers: local HDs and SSDs
* AWS: mounted EBS volumes
* GCE: mounted persistent volumes

Generally Quobyte Devices are formated (ext4) and mounted storage devices. 

Pick one device that will run the registry initially and run *qbootstrap* (bootstrapping needs to be done only once for the initial Quobyte setup). You need at least one registry, one metadata and one data device.

Further devices can be added to Quobyte in two ways:
1. On the host, tag the device with qmkdev.
2. Alternatively, provide a Mount disk to the Quobyte framework with Mesos' [Multiple Disk](http://mesos.apache.org/documentation/latest/multiple-disk/) support.

On AWS or GCE attach the devices across one or several instances.

Setup Client Mount
------------------

Create a directory /quobyte on all hosts that should receive a Quobyte mount point.

Configure Framework
-------------------

The framework is currently configured via command line flags. The following flags need to be used normally:

* *--zk*: the Zookeeper URL
* *--master*: the mesos master, usually as a Zookeeper URL
* *--port*: port of the framework's built-in HTTP server with a console and API (default 7888).
* *--docker_image*: the name of the Docker quobyte-server image (without version): [registry:port|name]/image
* *--framework_image*: the name of the Docker quobyte-mesos image, usually quobyte/quobyte-mesos:latest 
* *--registry_dns_name*: manually set the registry hosts, format: host:rpcport[,host:rpcport]. rpcport is usually 21000.
* *--restrict_hosts*: comma-separated list of full mesos-agent hostnames
* *--port_range_base*: the port range to use for service ports (10 ports will be allocated). Default is 21000.
* *--api_port*: the port for the JSON-RPC API. You can reach the API on http://api.quobyte.slave.mesos:<portno>. Default is 8889.
* *--webconsole_port*: the port of the Quobyte Webconsole. You can reach it on http://webconsole.quobyte.slave.mesos:<portno>. Default is 8888.
* *--framework_url*: the URL of the framework. Auto-generated if not set. You have to set it if you run the framework on changing hosts, like from Marathon, otherwise you get "invalid ExecutorInfo".

If something goes wrong, the *--reset* flag might be helpful, which deletes the framework's state on Zookeeper.

The default configuration relies on Mesos DNS. This solution is convenient but not perfect yet because of a limitation in Mesos DNS (see below).
If you want to name registry servers explicitely, set --mesos_dns_domain="" and --registry_dns_name to the hosts (+ :21000) on which you created registry devices.

Refer to the Quobyte documentation, specifically the Quick Start Guide on how to bootstrap registries.

Run on the command line
-----------------------

```
sudo docker run quobyte/quobyte-mesos:latest /opt/quobyte-mesos-cmd.sh --zk=zk.corp:2181 --master=zk://zk.corp:2181/mesos ...
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
      "image": "quobyte/quobyte-mesos:latest",
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
* Use dynamic port assignments, when Mesos knows how to co-allocate tcp and dns ports. (See [MESOS-4485](https://issues.apache.org/jira/browse/MESOS-4485)).
* Automatic /quobyte client mounts. This is implemented, but needs support from Mesos (See [MESOS-4717](https://issues.apache.org/jira/browse/MESOS-4717)).
* Automatic detection of new devices. Currently new devices made available on an already probed host are not updated in a probers container.
  - Workaround: When adding new devices manually shut down the given hosts prober. The framework will schedule a new prober for that host who will pick up old as well as the new devices.

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
