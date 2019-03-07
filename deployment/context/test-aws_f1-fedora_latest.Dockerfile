FROM fedora:latest

COPY ./test-aws_f1-sdk-install.sh /tmp/
COPY ./test-pip-install.sh /tmp/

RUN dnf install -y -q -e 0 python3-pip make gcc git sudo;\
/tmp/test-pip-install.sh;\
/tmp/test-aws_f1-sdk-install.sh;\
dnf erase -y --disableplugin=protected_packages make gcc git sudo;\
rm -rf /var/cache/dnf/*;
