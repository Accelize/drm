FROM centos:7

RUN yum install -y epel-release && \
yum install -y \
    gcc \
    git \
    make \
    python36-pip \
    sudo && \
python3 -m pip install -U --prefix='/usr' --no-cache-dir --disable-pip-version-check \
    pip \
    setuptools \
    wheel && \
pip3 install -U --prefix='/usr' --no-cache-dir --disable-pip-version-check \
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
