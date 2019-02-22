FROM ubuntu:16.04

COPY ./build-debian-install.sh /tmp/
COPY ./build-pip-install.sh /tmp/

RUN /tmp/build-debian-install.sh
