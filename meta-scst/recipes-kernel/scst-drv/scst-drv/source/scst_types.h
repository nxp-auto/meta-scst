 /*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef __SCST_TYPES_H
#define __SCST_TYPES_H

typedef struct {
	unsigned int scst_arg0;
	unsigned int scst_arg1;
} scst_args_t;

#define SCST_KM_MAGIC                    229

#define SCST_KM_IOCTL_RUN            _IOWR(SCST_KM_MAGIC, 0, scst_args_t *)
#define SCST_KM_IOCTL_PRE            _IOWR(SCST_KM_MAGIC, 1, scst_args_t *)

#ifndef MEMDEV_MAJOR
#define MEMDEV_MAJOR 0
#endif

#endif /* __SCST_TYPES_H */
