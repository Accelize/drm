FROM centos:7

RUN yum install -y epel-release && \
yum install -y \
    gcc \
    git \
    make \
    python36 \
    sudo && \
rm -f /usr/bin/python3 && \
ln -s /usr/bin/python36 /usr/bin/python3 && \
python3 -m ensurepip && \
ln -s /usr/local/bin/pip3 /usr/bin/pip3 && \
python3 -m pip install -U --no-cache-dir \
    pip \
    setuptools \
    wheel && \
pip3 install -U --no-cache-dir \
    pytest \
    requests \
    tox && \
git clone https://github.com/aws/aws-fpga /tmp/aws-fpga --depth 1 && \
source /tmp/aws-fpga/sdk_setup.sh && \
rm -Rf /tmp/aws-fpga && \
yum erase -y \
    gcc \
    git \
    make \
    sudo && \
rm -rf /var/cache/yum/*
