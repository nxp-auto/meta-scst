# Copyright 2023 NXP

SUMMARY = "SCST user space application"
LICENSE = "GPL-2.0"
LIC_FILES_CHKSUM = "file://LICENSE-GPL2.txt;md5=5dcdfe25f21119aa5435eab9d0256af7"

SRC_URI = "file://source/"

S = "${WORKDIR}/source"

EXTRA_OEMAKE_append = " DESTDIR=${D}"

do_install() {
        mkdir -p ${D}/home/root/scst
        install -d ${D}/home/root/scst
        cp -f ${S}/app_scst ${D}/home/root/scst
}

FILES_${PN} += "/home/root/scst"
INSANE_SKIP_${PN} = "ldflags"
INSANE_SKIP_${PN}-dev = "ldflags"
