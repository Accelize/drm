FROM fedora:latest

RUN dnf install -y -q -e 0 python3-pip python3-devel gcc rpm-build libcurl-devel doxygen jsoncpp-devel make gcc-c++ gnupg2 rpm-sign && \
python3 -m pip install -U -q --no-cache-dir pip setuptools wheel && \
pip3 install -U -q --no-cache-dir cython wheel cmake sphinx_rtd_theme sphinx tox breathe && \
ln -s /usr/bin/gpg2 /usr/bin/gpg && \
rm -rf /var/cache/dnf/*