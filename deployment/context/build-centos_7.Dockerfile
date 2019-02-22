FROM centos:7

COPY ./build-pip-install.sh /tmp/
COPY ./common-centos_7-python36-install.sh /tmp/

RUN yum install -y -q -e 0 epel-release;\
yum install -y -q -e 0 python36 python36-devel gcc rpm-build libcurl-devel doxygen jsoncpp-devel make gcc-c++ gnupg rpm-sign;\
/tmp/common-centos_7-python36-install.sh;\
/tmp/build-pip-install.sh;\
rm -rf /var/cache/yum/*;
