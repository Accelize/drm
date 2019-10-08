FROM python:3-slim

SHELL ["/bin/bash", "-c"]

RUN apt-get update && \
apt-get install -y --no-install-recommends \
    createrepo \
    doxygen \
    g++ \
    git \
    gnupg \
    libcurl4-openssl-dev \
    libjsoncpp-dev \
    make \
    pkg-config \
    reprepro \
    sudo && \
python3 -m pip install -U --no-cache-dir --disable-pip-version-check \
    pip \
    setuptools \
    wheel && \
pip3 install -U --no-cache-dir --disable-pip-version-check \
    awscli \
    breathe \
    cmake \
    cython \
    pytest \
    sphinx_rtd_theme  \
    tox && \
apt-get clean && \
apt-get autoremove -y --purge && \
rm -rf /var/lib/apt/lists/*
