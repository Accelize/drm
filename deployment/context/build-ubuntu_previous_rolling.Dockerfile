FROM ubuntu:cosmic

RUN apt-get update && \
apt-get install -y --no-install-recommends \
    doxygen \
    dpkg-dev \
    dpkg-sig \
    file \
    g++ \
    gnupg \
    libcurl4-openssl-dev \
    libjsoncpp-dev \
    make \
    pkg-config \
    python3-dev \
    python3-pip && \
python3 -m pip install -U --no-cache-dir \
    pip \
    setuptools \
    wheel && \
pip3 install -U --no-cache-dir \
    breathe \
    cmake \
    cython \
    sphinx_rtd_theme \
    tox && \
apt-get clean && \
apt-get autoremove -y --purge && \
rm -rf /var/lib/apt/lists/*
