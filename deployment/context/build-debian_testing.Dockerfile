FROM debian:testing-slim

COPY ./build-debian-install.sh /tmp/
COPY ./build-pip-install.sh /tmp/

RUN /tmp/build-debian-install.sh