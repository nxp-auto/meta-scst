###############################################################################
#
# Copyright 2023 NXP
#
# SPDX-License-Identifier: GPL-2.0
#
###############################################################################

obj-m := scst_drv.o
scst_drv-y := scst_main.o

EXTRA_CFLAGS += -I$(PWD)

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
