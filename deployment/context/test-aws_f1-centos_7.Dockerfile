FROM centos:7

RUN yum install -y -q -e 0 epel-release && \
yum install -y -q -e 0 python36 make gcc git sudo && \
ln -s /usr/bin/python36 /usr/bin/python3 && \
python3 -m ensurepip && \
ln -s /usr/local/bin/pip3 /usr/bin/pip3 && \
python3 -m pip install -U -q --no-cache-dir pip setuptools wheel && \
pip3 install -U -q --no-cache-dir tox pytest && \
git clone https://github.com/aws/aws-fpga /tmp/aws-fpga --depth 1 && \
source /tmp/aws-fpga/sdk_setup.sh && \
rm -Rf /tmp/aws-fpga && \
yum erase -y make gcc git sudo && \
rm -rf /var/cache/yum/*