FROM debian:testing-slim

SHELL ["/bin/bash", "-c"]

RUN apt-get update && \
apt-get install -y --no-install-recommends \
    gcc \
    git \
    libc-dev \
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
apt-get remove -y --purge \
    gcc \
    git \
    libc-dev \
    make && \
apt-get clean && \
apt-get autoremove -y --purge && \
rm -rf /var/lib/apt/lists/*
