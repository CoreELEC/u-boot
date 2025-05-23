/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "pmic.h"
#include "myprintf.h"

struct pmic_regulator *pmic_regulators[PMIC_MAXNUM] = { NULL1, NULL1, NULL1, NULL1, NULL1 };

int pmic_regulators_register(struct pmic_regulator *PmicRegulator, int *dev_id)
{
	for (int i = 0; i < PMIC_MAXNUM; i++) {
		if (pmic_regulators[i] == NULL1) {
			pmic_regulators[i] = PmicRegulator;
			*dev_id = i;
			return 0;
		}
	}
	printf("pmic_regulators has Greatered than PMIC_MAXNUM\n");
	return -1;
}

void pmic_i2c_init(int dev_id, struct pmic_i2c *pmic_i2c)
{
	((pmic_regulators[dev_id])->pmic_i2c_config)(pmic_i2c);
}

int pmic_regulator_ctrl(int dev_id, int id, int status)
{
	struct regulator_desc pmic_desc = ((pmic_regulators[dev_id])->rdev)[id];

	pmic_desc.ops->ctrl(&pmic_desc, status);
	return 0;
}

int pmic_regulator_set_voltage(int dev_id, int id, int sel)
{
	struct regulator_desc pmic_desc = ((pmic_regulators[dev_id])->rdev)[id];

	pmic_desc.ops->set_voltage(&pmic_desc, sel);
	return 0;
}

void pmic_osc(int dev_id, int status)
{
	((pmic_regulators[dev_id])->osc_ctrl)(status);
}
