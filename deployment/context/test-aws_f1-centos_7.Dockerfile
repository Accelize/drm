FROM centos:7

COPY ./common-centos_7-python36-install.sh /tmp/
COPY ./test-aws_f1-sdk-install.sh /tmp/
COPY ./test-pip-install.sh /tmp/

RUN yum install -y -q -e 0 epel-release;\
yum install -y -q -e 0 python36 make gcc git sudo;\
/tmp/common-centos_7-python36-install.sh;\
/tmp/test-pip-install.sh;\
/tmp/test-aws_f1-sdk-install.sh;\
yum erase -y make gcc git sudo;\
rm -rf /var/cache/yum/*;
