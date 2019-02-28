FROM debian:9-slim

COPY ./build-debian-install.sh /tmp/
COPY ./build-pip-install.sh /tmp/

RUN /tmp/build-debian-install.sh