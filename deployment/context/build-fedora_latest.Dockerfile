FROM fedora:latest

COPY ./build-pip-install.sh /tmp/

RUN dnf install -y -q -e 0 python3-pip python3-devel gcc rpm-build libcurl-devel doxygen jsoncpp-devel make gcc-c++ gnupg2 rpm-sign;\
/tmp/build-pip-install.sh;\
rm -rf /var/cache/dnf/*;
