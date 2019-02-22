#!/usr/bin/env bash

apt-get -qq update
apt-get -qq -o=Dpkg::Use-Pty=0 install -y --no-install-recommends python3-pip gcc make git sudo
/tmp/test-pip-install.sh
/tmp/test-aws_f1-sdk-install.sh
apt-get remove -y --purge gcc make git sudo
apt-get clean
apt-get autoremove -y --purge
rm -rf /var/lib/apt/lists/*
