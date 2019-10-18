FROM centos:8

RUN dnf install -y epel-release && \
dnf install -y 'dnf-command(config-manager)' && \
dnf config-manager --set-enabled epel-playground && \
dnf install -y --setopt=install_weak_deps=False --best \
    gcc \
    git \
    make \
    python3-pip \
    sudo && \
python3 -m pip install -U --no-cache-dir --disable-pip-version-check \
    pip \
    setuptools \
    wheel && \
pip3 install -U --no-cache-dir --disable-pip-version-check \
    pytest \
    requests \
    tox && \
git clone https://github.com/aws/aws-fpga /tmp/aws-fpga --depth 1 && \
source /tmp/aws-fpga/sdk_setup.sh && \
rm -Rf /tmp/aws-fpga && \
dnf erase -y make gcc git && \
rm -Rf /var/cache/dnf/*
