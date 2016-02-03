Quobyte DCOS Tutorial
=====================

How to run the Quobyte framework for Apache Mesos / Mesosphere DCOS on Amazon Web Services.

Requirements
------------

For this tutorial you will need the following:
* An Amazon AWS account 
* Knowledge how to access and download files from http://github.com
* An email address to register with Mesosphere for DCOS testing
* Basic knowledge on how to use a shell environment with sudo commands

Setting up AWS instances
------------------------

* Setup AWS Cloud Formation Template from Mesosphere
    * Follow https://mesosphere.com/amazon/setup/
* Install basic DCOS command line interface in order to be able to run DCOS cli commands later on
    * https://docs.mesosphere.com/administration/introcli/cli/

### Setup devices in AWS VMs

On each node that shall be part of the Quobyte setup a device has to be marked for Quobyte. Follow these steps to prepare the different nodes for Quobyte:

#### Fetch the Quobyte deployment tools you will need later on:

* Use DCOS ssh to log into the master node (see the DCOS cli documentation for details about this)

```
dcos node ssh --master-proxy --master
```

* Download the Quobyte deployment tools at https://github.com/quobyte/quobyte-deploy . These tools need to be run on the intended Quobyte nodes but we use the master for central download and distribution to the other DCOS slave nodes.

```
wget https://github.com/quobyte/quobyte-deploy/archive/master.zip
```

* Unpack the archive file to a local tmp directory

```
mkdir ./tmp
mv master.zip ./tmp/
cd ./tmp
unzip ./master.zip
```

* Now the required tools are available at ~/tmp/quobyte-deploy-master/tools/ on the master node and the different nodes can be prepared.


#### Distribute the deployment tools to three DCOS slave nodes that will later make up our Quobyte example installation

The tools on the master now can be distributed to the three nodes by secure copy. In order to get a list of available nodes used the DCOS cli:

```
dcos node 
```

From the resulting list select three slave nodes for further installation. Please note that you may choose an arbitrary number of nodes (at least one) but for simplicity reasons this tutorial uses three Quobyte nodes.

Now copy the deployment tools to all these nodes. Use the scp command and the IPs derived from the previous dcos node listing. An example line looks like this:

```
scp -r ~/tmp/quobte-deploy/tools core@10.0.0.185:/home/core/

```

Now the deployment tools are in place on the involved nodes, located at /home/core/tools .

#### Create local device mounts on three nodes that will be used as Quobyte hosts

On each of those nodes execute the following:

* Create /home/quobyte_device and /mnt/quobyte_device:

```
sudo mkdir /home/quobyte_device
sudo mkdir /mnt/quobyte_device
```

* Create a local device mount:

```    
sudo mount --bind /home/quobyte_device /mnt/quobyte_device
```

Please note that the mount --bind construction is used for testing purposes here. This will not be used with real devices or AWS EBS volumes used as Quobyte devices.

* Allow access for all users on volume mount points

```
sudo chmod 777 /mnt/quobyte_device
```

#### Bootstrap a single device in the cluster

Exactly one single device in the cluster is marked as the initial startup Quobyte Registry. This is done by running the qbootstrap tool on one of the previously set up devices. You can safely select one randomly.

```
sudo ~/tools/qbootstrap /mnt/quobyte_device
```

Please note that there will be a warning about qbootstrap beeing unable to change the owner of the mount point to quobyte:quobyte . This is expected in this tutorial.

#### Mark the other devices in the cluster as Metadata or Data devices

Following up the other previously created devices are marked as Metadata and Data devices for Quobyte on the other two nodes.

The Metadata device is marked with qmkdev on a second node

```
sudo ~/tools/qmkdev -t DATA /mnt/quobyte_device
```

The Data device is marked with qmkdev on a third node

```
sudo ~/tools/qmkdev -t DATA /mnt/quobyte_device
```

Please note that there will be a warning about qmkdev beeing unable to access the smartctl binary and, similar to qbootstrap, change the owner of the mount point to quobyte:quobyte . This is expected in this tutorial.

Furthermore, note that you can add more devices using qmkdev (**NOT qbootstrap!**) for all three types (Registry, Metadata, Data) if you prepared more nodes, e.g. in the local device mounts step above.

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

* Review the Quobyte framework details in the marathon webui of your DCOS AWS installation
* Open the Quobyte Webconsole
    * TBD: how to fetch / construct URL
* TBD: update operation
* TBD: stop operation
* TBD: (re-) start operation
