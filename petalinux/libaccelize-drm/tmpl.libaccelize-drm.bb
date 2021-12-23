#
# This file is the Accelize DRM library recipe.
#

SUMMARY = "Accelize DRM Library"
SECTION = "PETALINUX/apps"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://$${WORKDIR}/git/LICENSE;md5=$TMPL_LICENSE_MD5"

SRC_URI = "gitsm://github.com/Accelize/drm.git;protocol=git;$TMPL_GIT_OPTIONS"
SRCREV = "$TMPL_GIT_COMMIT"

DEPENDS += " \
    curl \
    jsoncpp \
"

S = "$${WORKDIR}/git"

PV = "${TMPL_PV}"

PR = "${TMPL_PR}"

DEPENDS += " \
    curl \
    jsoncpp \
"

INHIBIT_SYSROOT_STRIP = "1"

RM_WORK_EXCLUDE += "$${PN}"

EXTRA_OECMAKE+=" -DCMAKE_BUILD_TYPE=Debug"

inherit pkgconfig cmake

FILES_$${PN} += "$${libdir}/*"
