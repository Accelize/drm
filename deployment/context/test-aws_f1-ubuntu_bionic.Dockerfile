FROM ubuntu:bionic

SHELL ["/bin/bash", "-c"]

RUN apt-get -qq update && \
apt-get -qq -o=Dpkg::Use-Pty=0 install -y --no-install-recommends python3-pip gcc libc-dev make git sudo && \
python3 -m pip install -U -q --no-cache-dir pip setuptools wheel && \
pip3 install -U -q --no-cache-dir tox pytest && \
git clone https://github.com/aws/aws-fpga /tmp/aws-fpga --depth 1 && \
source /tmp/aws-fpga/sdk_setup.sh && \
rm -Rf /tmp/aws-fpga && \
apt-get remove -y --purge gcc libc-dev make git && \
apt-get clean && \
apt-get autoremove -y --purge && \
rm -rf /var/lib/apt/lists/*