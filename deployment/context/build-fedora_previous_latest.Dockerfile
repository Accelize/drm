FROM fedora:29

RUN dnf install -y --setopt=install_weak_deps=False --best \
    doxygen \
    gcc \
    gcc-c++ \
    gnupg2 \
    jsoncpp-devel \
    libcurl-devel \
    make \
    python3-pip \
    python3-devel \
    rpm-build \
    rpm-sign && \
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
ln -sf /usr/bin/gpg2 /usr/bin/gpg && \
rm -rf /var/cache/dnf/*