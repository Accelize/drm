FROM python:3-slim

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
    openssh-client \
    pkg-config \
    procps \
    reprepro \
    sudo && \
python3 -m pip install -U --no-cache-dir \
    pip \
    setuptools \
    wheel && \
pip3 install -U --no-cache-dir  \
    awscli \
    breathe \
    cmake \
    cython \
    pytest \
    'sphinx<2' \
    sphinx_rtd_theme  \
    tox && \
apt-get clean && \
apt-get autoremove -y --purge && \
rm -rf /var/lib/apt/lists/* && \
mkdir -p ~/.ssh && \
ssh-keyscan -t rsa,dsa -H github.com > ~/.ssh/known_hosts && \
chmod 600 ~/.ssh/known_hosts && \
echo "\
Host *\n\
  ConnectTimeout 10\n\
  ConnectionAttempts 10" > ~/.ssh/config
