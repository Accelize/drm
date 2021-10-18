#
# This file is the Accelize DRM library recipe.
#

SUMMARY = "Accelize DRM Library"
SECTION = "PETALINUX/apps"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${WORKDIR}/git/LICENSE;md5=e368f0931469a726e1017aaa567bd040"

SRC_URI = "gitsm://github.com/Accelize/drm.git;protocol=git;;branch=ptlx"
SRCREV = "ded62a793c89557769c0bbee637c3e3a9d14e7a7"

DEPENDS += " \
    curl \
    jsoncpp \
"

S = "${WORKDIR}/git"

PV = "2.5.4+ded62a7"

PR = "rc1.pl2020_2_2"

DEPENDS += " \
    curl \
    jsoncpp \
"

RM_WORK_EXCLUDE += "${PN}"

inherit pkgconfig cmake

FILES_${PN} += "${libdir}/*"
