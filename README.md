# meta-scst

## Overview

This is a Yocto meta layer for A53 SCST Linux integration demo based on S32G2/3 platform. It contains:

* Patch for ARM Trusted Firmware(ATF)
* Patch for Linux device tree
* Source code of a Linux Kernel module
* Linux user-space application 

## Tested HW & SW environment 

### HW

* S32G_VNP-RDB2
* S32G_VNP-RDB3
* S32G_VNP-EVB3

### SW

* Linux BSP34.0 
  * Linux v5.10.120
  * U-Boot v2020.04
  * ARM Trusted Firmware v2.5

* A53 SCST Library(NXP premium software)
  * A53 SCST Library for S32G RTM 2.0.0