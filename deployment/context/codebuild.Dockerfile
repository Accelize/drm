FROM ubuntu:18.04

RUN apt-get -qq update;\
apt-get -qq -o=Dpkg::Use-Pty=0 install -y --no-install-recommends\
    git\
    sudo\
    python3-dev python3-pip\
    g++ make libjsoncpp-dev libcurl-devel\
    doxygen\
    reprepro createrepo gnupg;\
python3 -m pip install -U pip setuptools wheel;\
pip3 install -U -q --no-cache-dir\
    cython cmake\
    awscli\
    tox pytest\
    sphinx breathe sphinx_rtd_theme;\
apt-get clean;\
apt-get autoremove -y --purge;\
rm -rf /var/lib/apt/lists/*;\
