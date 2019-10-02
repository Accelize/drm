FROM centos:8

RUN dnf install -y epel-release && \
dnf install -y --setopt=install_weak_deps=False --enablerepo=PowerTools --best \
    doxygen && \
dnf install -y --setopt=install_weak_deps=False --enablerepo=epel-playground --best \
    jsoncpp-devel && \
dnf install -y --setopt=install_weak_deps=False  --best \
    gcc \
    gcc-c++ \
    gnupg \
    libcurl-devel \
    make \
    python3-pip \
    python3-devel \
    rpm-build \
    rpm-sign && \
python3 -m pip install -U --no-cache-dir --disable-pip-version-check \
    pip \
    setuptools \
    wheel && \
pip3 install -U --no-cache-dir --disable-pip-version-check \
    breathe \
    cmake \
    cython \
    sphinx_rtd_theme \
    tox && \
rm -rf /var/cache/dnf/*
