FROM centos:7

RUN yum install -y epel-release && \
yum install -y \
    doxygen \
    gcc \
    gcc-c++ \
    gnupg \
    jsoncpp-devel \
    libcurl-devel \
    make \
    python36 \
    python36-devel \
    rpm-build \
    rpm-sign && \
rm -f /usr/bin/python3 && \
ln -s /usr/bin/python36 /usr/bin/python3 && \
python3 -m ensurepip && \
ln -s /usr/local/bin/pip3 /usr/bin/pip3 && \
python3 -m pip install -U --no-cache-dir \
    pip \
    setuptools \
    wheel && \
pip3 install -U --no-cache-dir \
    breathe \
    cmake \
    cython \
    'sphinx<2' \
    sphinx_rtd_theme \
    tox && \
rm -rf /var/cache/yum/*
