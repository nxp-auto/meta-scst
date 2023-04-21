# Copyright 2023 NXP

SUMMARY = "SCST Linux Kernel Module"
LICENSE = "GPL-2.0"
LIC_FILES_CHKSUM = "file://LICENSE-GPL2.txt;md5=5dcdfe25f21119aa5435eab9d0256af7"

inherit module

SRC_URI = "file://source/"

# Tell yocto not to bother stripping our binaries, especially the firmware
# since 'aarch64-fsl-linux-strip' fails with error code 1 when parsing the firmware
# ("Unable to recognise the format of the input file")
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_SYSROOT_STRIP = "1"

S = "${WORKDIR}/source"
EXTRA_OEMAKE_append = " KDIR=${STAGING_KERNEL_DIR} "

module_do_install() {
        mkdir -p ${D}/home/root/scst
        install -D ${S}/scst_drv.ko ${D}/home/root/scst/scst_drv.ko
}

#KERNEL_MODULE_AUTOLOAD += "scst-drv"

FILES_${PN} += "${base_libdir}/*"
FILES_${PN} += "${sysconfdir}/modules-load.d/*"

PROVIDES = "kernel-module-scst_drv${KERNEL_MODULE_PACKAGE_SUFFIX}"
RPROVIDES_${PN} = "kernel-module-scst_drv${KERNEL_MODULE_PACKAGE_SUFFIX}"

FILES_${PN} += "/home/root/scst"

COMPATIBLE_MACHINE = "s32g"

