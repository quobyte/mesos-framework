# Quobyte Mesos Framework Architecture

- obj is to clarify how the fw set's up the Quobyte infrastructure in the cluster

## Building Blocks

A Mesos based Quobyte system is based on the following building blocks:

- Mesos Cluster
    - cluster nodes have HDs and/or SSDs
    - preferably running Marathon
- quobyte-mesos framework
    - Runs as a Mesos framework task in a Docker container
    - Runs probes (created by the framework at runtime)
    - Runs Quobyte services as Mesos tasks
- Quobyte Docker Images
    - Three images available (framework, Quobyte server and Quobyte client)
    - quobyte-mesos framework docker image (quobyte/quobyte-mesos) is used to run the framework and is available publicly via Docker Hub.
    - Quobyte service image (quobyte-server) has to be downloaded from support.quobyte.com
    - Quobyte client image (quobyte-client), same as the Quobyte service image
- Quobyte services (run in Docker containers from the quobyte-server image)
    - Registry
    - Metadata
    - Data
    - API
    - Webconsole
    - Find more details about these services in the Quobyte manual

These building blocks are described in more Detail now.

### Mesos Nodes

- Have HDs and/or SSDs that may be used for Quobyte
    - Small device files on a device's filesystem mark these devices to be used for a specific Quobyte service (for details on marking devices please see the Quobyte manual)
    - Marking these devices defines the structure of your Quobyte setup, e.g. how many Quobyte Registry-, Metadata- and Data services will be run.
- Host the Quobyte Mesos Framework, prober and service tasks
    - Quobyte services in a container access the device(s) on the the baremetal Mesos node

### quobyte-mesos Framework

- Scheduler for running Quobyte in the Mesos cluster
- Runs as a framework within Mesos
- Typically run via Marathon
- creates different types of tasks which are run on the cluster
    - Prober tasks run on all nodes to locate device marked for Quobyte
    - Quobyte service tasks, some on specific nodes that are hosting Quobyte devices(Registry-, Metadata-, Data services), others on any available host (API-, Webconsole service)
- Monitors overall Quobyte system state 

### Quobyte Services

- Run as Mesos tasks in docker containers (one per service instance)
- Services that require host devices are Registry, Metadata and Data
    - A Quobyte service is started on a host if one or more devices for this service are present. For example:
        - if one Registry device is present on a host one Registry service is run.
        - If two Data devices are present one Data service is run.
        - If one device for each Quobyte service type is present all three services are run in individual containers on this host.
        - and so on.
    - A single instance of each of these three services is the bare minimum required to run Quobyte in the cluster
- Other Quobyte services that are run on any available node are API and Webconsole as they do not require specific devices

## Startup Overview

Setting up a Quobyte system in your Mesos cluster follows the following steps:

### 1) Setup Devices

Marking devices for specific Quobyte services is an important design decision. Thus consult the Quobyte user manual on how many Registries, Metadata and Data services you will set up in your cluster and where to place them. Review the section on capacity planning for this. When this has been planned the according devices are marked using the [Quobyte deployment tools](https://github.com/quobyte/quobyte-deploy). 

### 2) Fetch Quobyte Docker Images
While the frameworks Docker image is available via Docker Hub and can be retrieved at runtime the images for the Quobyte services and client have to be downloaded from [support.quobyte.com](https://support.quobyte.com/). Please contact [info@quobyte.com](mailto:info@quobyte.com) for access to our repository.
After you have downloaded the Docker images these can either be distributed onto the nodes of your cluster or loaded into a private Docker registry.

### 3) Start Quobyte Mesos Framework
The quobyte-mesos framework is typically run via Marathon. Upon startup the framework starts creating prober tasks that run on the available Mesos nodes and locate devices previously marked for Quobyte. These findings are accumulated by the framework. New devices can be added and marked in the cluster at runtime and will be integrated into the Quobyte system upon detection by a prober and subsequent service startup initiated by the framework.

### 4) Run Quobyte
Now you can start/stop/upgrade the Quobyte installation in your cluster as described in the Readme.
Following up on the devices found by the prober tasks the framework starts Quobyte service tasks on the identified Mesos nodes. The exact service types are chosen depending on the devices found by the probers. The availablility of these tasks is monitored and continously reconciled by the framework. The different Registry-, Metadata-, and Data services connect via the internal Quobyte mechanisms and form the running Quobyte platform. Webconsole and API services can be used to interact with the system.
