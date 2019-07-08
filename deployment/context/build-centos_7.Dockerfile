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
    python36-pip \
    python36-devel \
    rpm-build \
    rpm-sign && \
python3 -m pip install -U --prefix='/usr' --no-cache-dir \
    pip \
    setuptools \
    wheel && \
pip3 install -U --prefix='/usr' --no-cache-dir \
    breathe \
    cmake \
    cython \
    sphinx_rtd_theme \
    tox && \
rm -rf /var/cache/yum/*
