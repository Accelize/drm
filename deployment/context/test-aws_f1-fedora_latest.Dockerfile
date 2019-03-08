FROM fedora:latest

RUN dnf install -y -q -e 0 python3-pip make gcc git sudo && \
python3 -m pip install -U -q --no-cache-dir pip setuptools wheel && \
pip3 install -U -q --no-cache-dir tox pytest && \
git clone https://github.com/aws/aws-fpga /tmp/aws-fpga --depth 1 && \
source /tmp/aws-fpga/sdk_setup.sh && \
rm -Rf /tmp/aws-fpga && \
dnf erase -y make gcc git && \
rm -Rf /var/cache/dnf/*