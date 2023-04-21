 /*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "scst_types.h"

#define DRV_NAME 	"/dev/scst_drv"

const unsigned int atomic_test_results[60] = {
	0x5F36A181, 0x35FA0B8, 0x6271A172, 0x779840C3, 0x34D2656B,
	0x8585A708, 0xDBD415E4, 0xF7E94477, 0x274B882D, 0xAD044F41,
	0x3CB07482, 0xB2F5E0C3, 0x0DFF8AA7, 0xC690EB27, 0x64EE1D3D,
	0x336D449B, 0x0C69F519, 0xA8C916F7, 0x70C77FF1, 0xE641875B,
	0x136403D7, 0xFF478980, 0x8DBDE746, 0xC17AB755, 0x5254F6B7,
	0xCBE69022, 0xD3B607BF, 0x4FE17586, 0x9E602696, 0x37148BC3,
	0x7586C644, 0x5957C03B, 0x34BA4187, 0xB3C0184C, 0x9EE67269,
	0x64583932, 0x90D79E78, 0x38972278, 0x758702BC, 0xA5618CF6,
	0xA23BCF0A, 0x4533355D, 0xA866E84C, 0xD580D175, 0x7A4C4F75,
	0x3A594CB6, 0x78C75F66, 0x1DEFC94, 0x5C380774, 0x1796A854,
	0x17EE1095, 0xF4A05191, 0x23FF50B7, 0x6B7D98E8, 0xBF3B8A2F,
	0x3886B60A, 0x245B89D6, 0xCDB70CCF, 0x5CA2A5C2, 0x6C165C62
};

unsigned int scst_execute_tests(const int fd, unsigned int start_index, unsigned int end_index)
{
	scst_args_t scst_arg;

	scst_arg.scst_arg0 = start_index;
	scst_arg.scst_arg1 = end_index;

	if (ioctl(fd, SCST_KM_IOCTL_RUN, &scst_arg) < 0) {
		printf("Set and run scst failed\n");
		return -1;
	}

	return scst_arg.scst_arg0;
}

int main(void)
{
	int fd = 0;
	unsigned int scst_res = 0;
	unsigned int expected_signature = 0;
	unsigned int testresults = 0;

	unsigned int start_id = 0;
	unsigned int end_id = 59;

	/* Open scst dev */
	fd = open(DRV_NAME, O_RDWR);
	if (fd < 0) {
		printf("Open dev %s error!\n", DRV_NAME);
		return -1;
	}

	if (ioctl(fd, SCST_KM_IOCTL_PRE) < 0) {
		printf("Prepare scst execute env failed\n");
		return -1;
	}

	scst_res = 0;

	printf("[INFO] Perform every atomic test one by one ...\n");
	for (int i = start_id; i <= end_id; i++) {
		usleep(200000);
		scst_res = scst_execute_tests(fd, i, i);
		expected_signature ^= atomic_test_results[i];

		if (atomic_test_results[i] == scst_res) {
			printf("[INFO] -- Test #%02d passed! [expected: 0x%08X == result: 0x%08X]\n", i, atomic_test_results[i], scst_res);			
		} else {
			printf("[ERROR] -- Test #%02d failed! [expected: 0x%08X == result: 0x%08X]\n", i, atomic_test_results[i], scst_res);	
		}			
	}

	printf("\n[INFO] Perform all atomic tests one time ...\n");
	scst_res = scst_execute_tests(fd, start_id, end_id);
	if (expected_signature == scst_res) {	
		printf("[INFO] -- Test all passed! [expected: 0x%08X == result: 0x%08X]\n", expected_signature, scst_res);			
	} else {
		printf("[ERROR] -- Test failed! [expected: 0x%08X == result: 0x%08X]\n", expected_signature, scst_res);	
	}
	
	return 0;
}
