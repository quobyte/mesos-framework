AWS run Tutorial
=================

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
* install basic DCOS
    * https://docs.mesosphere.com/administration/introcli/cli/
* Setup devices in AWS VMs
    * QB deployment tools are available at https://github.com/quobyte/quobyte-deploy
    * fetch qbootstrap tool for initial registry host
    * fetch qmkdev tool on all hosts
    * mount --bind /home/quobyte_device /mnt/quobyte_device on all hosts
        * Instead of local mount --bind persistent EBS volumes can be used
        * For more information see the Quobyte manual, especially the Quickstart guide.
    * Allow access for all users on volume mount points

        ``` chmod 777 /mnt/quobyte_device ```

    * Create Registry with qbootstrap on initial registry host / Node

        ```sudo qbootstrap /mnt/quobyte_device```

    * Metadata with qmkdev on a second

        ```sudo qmkdev -t DATA /mnt/quobyte_device```

    * Data with qmkdev on a third

        ```sudo qmkdev -t DATA /mnt/quobyte_device```
    * You can add more devices using qmkdev (**NOT qbootstrap!**) for all three types (Registry, Metadata, Data) according to your requirements. This marks all devices to be used by the quobyte sytem.

* Install Quobyte DCOS package

```bash
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
