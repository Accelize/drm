#!/usr/bin/env bash

apt-get -qq update
apt-get -qq -o=Dpkg::Use-Pty=0 install -y --no-install-recommends python3-dev python3-pip libjsoncpp-dev dpkg-dev g++ pkg-config file doxygen make libcurl4-openssl-dev dpkg-sig gnupg
/tmp/build-pip-install.sh
apt-get clean
apt-get autoremove -y --purge
rm -rf /var/lib/apt/lists/*
