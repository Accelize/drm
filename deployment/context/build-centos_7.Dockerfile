FROM centos:7

RUN yum install -y -q -e 0 epel-release && \
yum install -y -q -e 0 python36 python36-devel gcc rpm-build libcurl-devel doxygen jsoncpp-devel make gcc-c++ gnupg rpm-sign && \
ln -s /usr/bin/python36 /usr/bin/python3 && \
python3 -m ensurepip && \
ln -s /usr/local/bin/pip3 /usr/bin/pip3 && \
python3 -m pip install -U -q --no-cache-dir pip setuptools wheel && \
pip3 install -U -q --no-cache-dir cython wheel cmake sphinx_rtd_theme sphinx tox breathe && \
rm -rf /var/cache/yum/*