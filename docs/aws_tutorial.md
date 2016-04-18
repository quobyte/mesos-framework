# Quobyte DCOS Installation Tutorial

How to install the Quobyte framework for Apache Mesos / Mesosphere DCOS on Amazon Web Services.

## Requirements

For this tutorial you will need the following:

* An Amazon AWS account 
* Knowledge how to access and download files from http://github.com
* An email address to register with Mesosphere for DCOS testing
* Basic knowledge on how to use a shell environment with sudo commands
* Note: this tutorial requires **Quobyte version 1.2.5 or later**

## Setting up AWS instances

* Setup AWS Cloud Formation Template from Mesosphere
    * Follow https://mesosphere.com/amazon/setup/
* Install basic DCOS command line interface in order to be able to run DCOS cli commands later on
    * https://docs.mesosphere.com/administration/introcli/cli/

### Setup devices in AWS VMs

On each node that shall be part of the Quobyte setup a device has to be marked for Quobyte. In this tutorial we use a three node Quobyte setup. Follow these steps to prepare the different nodes for Quobyte:

#### Fetch the Quobyte deployment tools you will need later on:

* Use DCOS ssh to log into the master node (see the DCOS cli documentation for details about this)

```
dcos node ssh --master-proxy --master
```

* Download the Quobyte deployment tools at https://github.com/quobyte/quobyte-deploy . These tools need to be run on the intended Quobyte nodes but we use the master for central download and distribution to the other DCOS slave nodes.
* Unpack the archive file to a local tmp directory
* Download the ``quobyte-server-image_VERSION.tar.bzip2`` file from your Quobyte repository to the master node

(You can use the following example via copy paste if you replace ``YOUR_REPO_ID`` with the ID from your repo url and ``VERSION`` in the file name with the latest version available in your repo.)
```
mkdir ./tmp
cd ./tmp
wget https://codeload.github.com/quobyte/quobyte-deploy/zip/master
unzip ./quobyte-deploy-master.zip
wget https://support.quobyte.com/repo/2/YOUR_REPO_ID/quobyte-docker/quobyte-server-image_VERSION.tar.bzip2
```

* Now the required tools (~/tmp/quobyte-deploy-master/tools/) and image (~/tmp/quobyte-server-image_VERSION.tar.bzip2) are available on the master node and the different slave nodes can be prepared.


#### Distribute the deployment tools and docker image to three DCOS slave nodes that will later make up our Quobyte example installation

The tools and image on the master can now be distributed to the nodes by secure copy. In order to get a list of available nodes used the DCOS cli:

```
dcos node 
```

Now copy the image to all these nodes. Use the scp command and the IPs derived from the previous dcos node listing (Alternatively you can deploy a private docker registry in your DCOS cluster). An example looks like this:

```
scp -r ~/tmp/quobyte-deploy-master/tools core@10.0.0.185:/home/core/
scp ~/tmp/quobyte-server-image_1.2.3.tar.bzip2 core@10.0.0.185:/home/core/

```

Now the deployment tools and image are in place on the involved nodes, located at /home/core/tools for the Quobyte tools.

#### Create local device mounts on three nodes that will be used as Quobyte hosts

From the list of nodes select three slave nodes for further installation. Please note that you may choose an arbitrary number of nodes (at least one) but for simplicity reasons this tutorial uses three Quobyte nodes.

For each of those nodes execute the following:

* Log into the node via ssh

```
ssh <node ip address>
```

Then take the following steps (as shown in the example below):
* Create /home/quobyte_device and /mnt/quobyte_device:
* Create a local device mount:
* Allow access for all users on volume mount points
* Import the docker image into the local docker daemon repository

(You can copy & paste the following example if you adopt ``VERSION`` to your docker image file version.)
```
sudo mkdir /home/quobyte_registry
sudo mkdir /mnt/quobyte_registry
sudo mount --bind /home/quobyte_registry /mnt/quobyte_registry
sudo mkdir /home/quobyte_metadata
sudo mkdir /mnt/quobyte_metadata
sudo mount --bind /home/quobyte_metadata /mnt/quobyte_metadata
sudo mkdir /home/quobyte_data
sudo mkdir /mnt/quobyte_data
sudo mount --bind /home/quobyte_data /mnt/quobyte_data
sudo chmod 777 /mnt/quobyte_registry
sudo chmod 777 /mnt/quobyte_metadata
sudo chmod 777 /mnt/quobyte_data
docker load -i quobyte-server-image_VERSION.tar.bzip2
```

Please note that the mount --bind construction is used for testing purposes here. This will not be used with real devices or AWS EBS volumes that are used as Quobyte devices.

You can verify the loaded docker images by listing locally available images, there should be at least two entries available from the loaded docker image archive. Both have the same IMAGE ID but different tags (a release tag and a hash key tag). List the images buy running:

```
docker images
```


#### Bootstrap a single device in the cluster

Exactly one single device in the cluster is marked as the initial startup Quobyte Registry. This is done by running the qbootstrap tool on one of the previously set up quobyte_registry mounts on one single node. You can safely select one randomly.

* Log into the selected node that will host the bootstrap Quobyte registry.
* Run the qbootstrap tool on the ``/mnt/quobyte_registry`` mount

```
sudo ~/tools/qbootstrap /mnt/quobyte_registry
```

Please note that there will be a warning about qbootstrap beeing unable to access the smartctl binary and not beeing able to change the owner of the mount point to quobyte:quobyte . This is expected in this tutorial.

#### Mark all other devices in the cluster according to their type

Following up the other previously created devices are marked as Metadata and Data devices for Quobyte on the node bootstrapped above, as well as Registry, Metadata and Data devices on the other two nodes.

* Mark Metadata and Data devices on the current node (where ``/mnt/quobyte_registry`` has already been bootstrapped)
* Log into the other two nodes and mark Registry, Metadata and Data devices using the ``qmkdev`` tool

The devices are marked according to type with qmkdev as follows:
```
sudo ~/tools/qmkdev -t REGISTRY /mnt/quobyte_registry
sudo ~/tools/qmkdev -t METADATA /mnt/quobyte_metadata
sudo ~/tools/qmkdev -t DATA /mnt/quobyte_data
```

Please note that there will be a warning about qmkdev beeing unable to access the smartctl binary and change the owner of the mount point to quobyte:quobyte (similar to qbootstrap). This is expected in this tutorial.

* You may exit from the ssh remote shells, execute exit on all nodes:

```
exit
```


### Install Quobyte DCOS package

You can now run the DCOS package installation for Quobyte, like in this example:

```
~>dcos package install quobyte
Installing Quobyte.
Required Mesos version for this package is 0.25.0
In order to start the Quobyte framework a minimum of 1 cpu share and 128MB RAM need to be available.
Please ensure your devices have been set up (see https://github.com/quobyte/mesos-framework#setup-devices)
Continue installing? [yes/no] yes
Installing Marathon app for package [quobyte] version [1.2]
Installing CLI subcommand for package [quobyte] version [1.2]
New command available: dcos quobyte
Quobyte has been successfully installed!
Documentation: https://support.quobyte.com/
```

Upon installation the framework will start probing and locating the devices created earlier in this tutorial. This will take a minute. You can review the process in Mesos at ``http://MASTER_NODE_NAME`` and the results at the frameworks web ui located at ``http://MASTER_NODE_NAME/service/quobyte/``. This will show the devices located on the different nodes but currently no services running.

### Start Quobyte on your cluster

After the Quobyte DCOS package has been installed the Quobyte Mesos framework in your cluster starts probing the cluster for the Quobyte devices you marked earlier. This may take a few minutes. With the devices detected you can now start the Quobyte services.

* Note/copy your DCOS master nodes host name for later use in the following steps
* Start up the Quobyte services by running the following command via DCOS:

(You can copy & paste the following command if you set the ``VERSION`` from the previously installed Quobyte docker image file and ``MASTER_NODE_NAME`` according to your DCOS master host name.)
```
dcos quobyte start --release=VERSION --host=http://MASTER_NODE_NAME/service/quobyte/
```

Please note that this command currently returns an error code 502, although the framework is starting correctly. This issue will be removed in a future release.

Following this command the Quobyte framework starts running the different Quobyte services on the involved AWS nodes. These are shown in the Mesos UI at ``http://MASTER_NODE_NAME/mesos/`` .

### Use and manage your Quobyte cluster
Now you can user Quobyte in your DCOS cluster. In order to do so you can use the Quobyte cli tools from within DCOS or access the Quobyte Webconsole service. This can be done right away by using a SSH tunnel with forwarding. In order to do so find the address of the proxy node. It can e.g. be seen when using dcos to log into the master node:

```
[kaisers@kaisers ~>dcos node ssh --master-proxy --master
Running `ssh -A -t core@52.27.103.24 ssh -A -t core@10.0.6.39`
<find the address here: ^^^^^^^^^^^^>
```

Using this ``proxy address `` you can now ssh into your cluster:

```
ssh -A -L <local_port>:<internal_node>:<webconsole_port> core@<proxy_address>
```

With this tunnel in place you can access the Quobyte Webconsole service at ``http://localhost:<localport>/``.

Access to the Quobyte API service can be achieved by similar means.

<!--

# The following config currently does not work in DCOS but should in the future. Recheck when public UI access has been updated in DCOS.

#### Publicly accessing the Quobyte Webconsole service

In order to access the Quobyte Webconsole service from outside the cluster you can deploy a service instance via marathon onto a public slave. Use the following JSON config, set the required Quobyte version in the ``image`` tag and run this via marathon. Make sure the ``quobyte-server`` is available to the docker daemons.

```
{
   "id":"quobyte-webconsole",
   "cmd":"export QUOBYTE_SERVICE=webconsole && export QUOBYTE_REGISTRY=_registry._tcp.quobyte.mesos && export QUOBYTE_RPC_PORT=21008 && export QUOBYTE_HTTP_PORT=21009 && export QUOBYTE_WEBCONSOLE_PORT=8888 && export HOST_IP=$(dig +short $HOSTNAME) && /opt/main.sh",
   "instances":1,
   "cpus":0.5,
   "mem":512.0,
   "acceptedResourceRoles": ["slave_public"],
   "ports":[8888],
   "requirePorts": true,
   "container": {
    "type": "DOCKER",
    "docker": {
      "forcePullImage": false,
      "image": "quobyte/quobyte-server:<VERSION>",
      "network": "HOST",
      "privileged": true
    }
  }
}

```

Deploy this configuration via ``dcos marathon``:

```
dcos marathon app add /path/to/webconsole/json/file
```

The same holds true if you want to access the Quobyte API from outside the cluster. Apply belows configuration similar to the Webconsole service described above.

```
{
   "id":"quobyte-api",
   "cmd":"export QUOBYTE_SERVICE=api && export QUOBYTE_REGISTRY=_registry._tcp.quobyte.mesos && export QUOBYTE_RPC_PORT=21006 && export QUOBYTE_HTTP_PORT=21007 && export HOST_IP=$(dig +short $HOSTNAME) && /opt/main.sh",
   "instances":1,
   "cpus":0.1,
   "mem":512.0,
   "acceptedResourceRoles": ["slave_public"],
   "ports":[21006],
   "requirePorts": true,
   "container": {
    "type": "DOCKER",
    "docker": {
      "forcePullImage": false,
      "image": "quobyte/quobyte-server:<VERSION>",
      "network": "HOST",
      "privileged": true
    }
  }
}

```
-->