FROM ubuntu:bionic

RUN apt-get -qq update && \
apt-get -qq -o=Dpkg::Use-Pty=0 install -y --no-install-recommends python3-dev python3-pip libjsoncpp-dev dpkg-dev g++ pkg-config file doxygen make libcurl4-openssl-dev dpkg-sig gnupg && \
python3 -m pip install -U -q --no-cache-dir pip setuptools wheel && \
pip3 install -U -q --no-cache-dir cython wheel cmake sphinx_rtd_theme sphinx tox breathe && \
apt-get clean && \
apt-get autoremove -y --purge && \
rm -rf /var/lib/apt/lists/*