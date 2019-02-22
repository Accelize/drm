FROM ubuntu:rolling

COPY ./test-aws_f1-sdk-install.sh /tmp/
COPY ./test-pip-install.sh /tmp/
COPY ./test-debian-install.sh /tmp/

RUN /tmp/test-debian-install.sh
