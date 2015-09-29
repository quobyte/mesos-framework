FROM centos:centos7

RUN rpm --import http://epel.mirrors.ovh.net/epel/RPM-GPG-KEY-EPEL-7
RUN rpm -Uvh http://epel.mirrors.ovh.net/epel/7/x86_64/e/epel-release-7-5.noarch.rpm

RUN yum update -y
RUN yum install -y gflags protobuf libmicrohttpd subversion-libs
ADD scheduler/quobyte-mesos /opt/quobyte-mesos
ADD executor/executor.tar.gz /opt/executor.tar.gz
ADD quobyte-mesos.sh /opt/quobyte-mesos.sh
ADD thirdparty/mesos/build/src/.libs/libmesos-0.24.1.so /opt/libmesos-0.24.1.so

ENTRYPOINT ["/opt/quobyte-mesos.sh"]
