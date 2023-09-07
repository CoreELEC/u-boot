/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include <common.h>
#include <pwm.h>
#include <register.h>
#include <task.h>

#define pwm_readl(reg) (*((volatile uint32_t *)(reg)))
#define CLK_24M 24000000UL
#define USEC_PER_SEC 1000000000ULL

#define DIV_ROUND_UP(x, y) (((x) + ((y)-1)) / (y))
#define DIV_ROUND_NEAREST(x, y) (((x) + ((y) / 2)) / (y))
#define DIV_ROUND_CLOSEST(x, divisor)                                                              \
	({                                                                                         \
		typeof(x) __x = x;                                                                 \
		typeof(divisor) __d = divisor;                                                     \
		(((typeof(x)) -1) > 0 || ((typeof(divisor)) -1) > 0 || (__x) > 0) ?              \
			(((__x) + ((__d) / 2)) / (__d)) :                                          \
			(((__x) - ((__d) / 2)) / (__d));                                           \
	})

/*pwm register att*/
struct xPwmMesonRegs {
	uint32_t dar; /* A Duty Register */
	uint32_t dbr; /* B Duty Register */
	uint32_t miscr; /* misc Register */
	uint32_t dsr; /*DS Register*/
	uint32_t tr; /*times Register*/
	uint32_t da2r; /* A2 Duty Register */
	uint32_t db2r; /* B2 Duty Register */
	uint32_t br; /*Blink Register*/
};

static void prvPwmEnterCritical(UBaseType_t *uxIsr)
{
	if (xPortIsIsrContext())
		*uxIsr = taskENTER_CRITICAL_FROM_ISR();
	else {
		taskENTER_CRITICAL();
		*uxIsr = 0;
	}
};

static void prvPwmExitCritical(UBaseType_t uxSaveIsr)
{
	if (xPortIsIsrContext())
		taskEXIT_CRITICAL_FROM_ISR(uxSaveIsr);
	else {
		taskEXIT_CRITICAL();
		uxSaveIsr = 0;
	}
};

static void prvPwmRegWrite(uint32_t addr, uint32_t mask, uint32_t val)
{
	UBaseType_t uxSavedIsr;

	prvPwmEnterCritical(&uxSavedIsr);
	REG32_UPDATE_BITS(addr, mask, val);
	prvPwmExitCritical(uxSavedIsr);
}

static struct xPwmMesonRegs *prvDeviceToRegs(struct xPwmMesondevice *pwm)
{
	return (struct xPwmMesonRegs *)pwm->chip->addr;
}

static uint32_t vPwmMesonVolttoDuty(struct xPwmMesonVoltage *vtable, uint32_t vtable_size,
				    uint32_t voltage_mv)
{
	uint32_t i;

	if ((voltage_mv < vtable[0].Voltage_mv) ||
	    (voltage_mv > vtable[vtable_size - 1].Voltage_mv)) {
		printf("volt: %dmv out of set range [%dmv - %dmv]\n", voltage_mv,
		       vtable[0].Voltage_mv, vtable[vtable_size - 1].Voltage_mv);
		return 0;
	}

	/* Get voltage up */
	for (i = 0; i < vtable_size; i++)
		if (voltage_mv <= vtable[i].Voltage_mv)
			return vtable[i].Duty_reg;

	return 0;
}

static int32_t vPwmMesonDutytoVolt(struct xPwmMesonVoltage *vtable, uint32_t vtable_size,
				   uint32_t duty)
{
	uint32_t i;

	for (i = 0; i < vtable_size; i++)
		if (duty == vtable[i].Duty_reg)
			return vtable[i].Voltage_mv;

	return -1;
}

void vPwmMesonPwmDebug(struct xPwmMesondevice *pwm)
{
	uint32_t tmp = 0;
	uint32_t i;
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);

	printf("pwm debug info chip_id = %d channel_id = %d addr = %p\n", pwm->chip->chip_id,
	       pwm->hwpwm, reg);

	for (i = 0; i <= 7; i++) {
		tmp = pwm_readl(&reg->dar + i);
		printf("reg-> = 0x%x\n", tmp);
	}
}

void vPwmConstantDisable(struct xPwmMesondevice *pwm)
{
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);

	switch (pwm->hwpwm) {
	case MESON_PWM_0:
	case MESON_PWM_2:
		prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 28), 0);
		break;

	case MESON_PWM_1:
	case MESON_PWM_3:
		prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 29), 0);
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		break;
	}
}

void vPwmConstantEnable(struct xPwmMesondevice *pwm)
{
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);

	switch (pwm->hwpwm) {
	case MESON_PWM_0:
	case MESON_PWM_2:
		prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 28), (1 << 28));
		break;

	case MESON_PWM_1:
	case MESON_PWM_3:
		prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 29), (1 << 29));
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		break;
	}
}

static uint32_t prvPwmGetPolarity(struct xPwmMesondevice *pwm)
{
	uint32_t tmp, val;
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);

	switch (pwm->hwpwm) {
	case MESON_PWM_0:
	case MESON_PWM_2:
		val = 0x1 << 26;
		break;

	case MESON_PWM_1:
	case MESON_PWM_3:
		val = 0x1 << 27;
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		return 0;
	}

	tmp = pwm_readl(&reg->miscr);
	tmp = tmp & val;
	if (tmp == 0)
		return 0;
	else
		return 1;
}

static void prvPwmMesonClockSet(struct xPwmMesondevice *pwm)
{
	switch (pwm->hwpwm) {
	case MESON_PWM_0:
	case MESON_PWM_2:
		prvPwmRegWrite(pwm->chip->clk_addr, ((0x3 << 9) | (0x1 << 8)), (0x1 << 8));
		break;

	case MESON_PWM_1:
	case MESON_PWM_3:
		prvPwmRegWrite(pwm->chip->clk_addr, ((0x3 << 25) | (0x1 << 24)), (0x1 << 24));
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		break;
	}
}

static int32_t prvPwmCalc(struct xPwmMesondevice *pwm, uint32_t duty, uint32_t period)
{
	uint32_t pre_div, cnt, duty_cnt;
	uint32_t fin_freq, fin_ns;
	int32_t inv;

	inv = prvPwmGetPolarity(pwm);
	if (inv)
		duty = period - duty;

	/*fin_freq = CLK_24M; */
	fin_freq = 24000000;
	fin_ns = USEC_PER_SEC / fin_freq;

	for (pre_div = 0; pre_div < 0x7f; pre_div++) {
		cnt = DIV_ROUND_CLOSEST(period, fin_ns * (pre_div + 1));
		if (cnt <= 0xffff)
			break;
	}

	if (pre_div == 0x7f) {
		printf("unable to get period pre_div\n");
		return -1;
	}

	if (duty == period) {
		pwm->pwm_pre_div = pre_div;
		pwm->pwm_hi = cnt - 2;
		pwm->pwm_lo = 0;
		vPwmConstantEnable(pwm);
	} else if (duty == 0) {
		pwm->pwm_pre_div = pre_div;
		pwm->pwm_hi = 0;
		pwm->pwm_lo = cnt - 2;
		vPwmConstantEnable(pwm);
	} else {
		/* Then check is we can have the duty with the same pre_div */
		duty_cnt = DIV_ROUND_CLOSEST(duty, fin_ns * (pre_div + 1));
		if (duty_cnt > 0xffff) {
			printf("unable to get duty cycle\n");
			return -1;
		}

		pwm->pwm_pre_div = pre_div;

		if (duty_cnt == 0) {
			cnt = (cnt < 2 ? 2 : cnt);
			pwm->pwm_hi = 0;
			pwm->pwm_lo = cnt - 2;
		} else if (cnt == duty_cnt) {
			duty_cnt = (duty_cnt < 2 ? 2 : duty_cnt);
			pwm->pwm_hi = duty_cnt - 2;
			pwm->pwm_lo = 0;
		} else {
			pwm->pwm_hi = duty_cnt - 1;
			pwm->pwm_lo = cnt - duty_cnt - 1;
		}
		vPwmConstantDisable(pwm);
	}

	return 0;
}

static void prvMesonConfig(struct xPwmMesondevice *pwm)
{
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);

	switch (pwm->hwpwm) {
	case MESON_PWM_0:
		/*set div and clock enable */
		/* If using clktree */
		if (pwm->chip->clk_addr) {
			if (pwm->chip->channel_separated && !pwm->chip->even_channel)
				prvPwmRegWrite(pwm->chip->clk_addr, ((0xff << 16) | (1 << 24)),
				       ((pwm->pwm_pre_div << 16) | (1 << 24)));
			else
				prvPwmRegWrite(pwm->chip->clk_addr, ((0xff << 0) | (1 << 8)),
				       ((pwm->pwm_pre_div << 0) | (1 << 8)));
		} else {
			prvPwmRegWrite((uint32_t)&reg->miscr,
				       ((0x3 << 4) | (0x7f << 8) | (1 << 15)),
				       (((pwm->pwm_pre_div << 8)) | (1 << 15)));
		}

		/*set duty */
		prvPwmRegWrite((uint32_t)&reg->dar, 0xffffffff,
			       ((pwm->pwm_hi << 16) | (pwm->pwm_lo)));
		break;

	case MESON_PWM_1:
		/*set div and clock enable */
		if (pwm->chip->clk_addr) {
			prvPwmRegWrite(pwm->chip->clk_addr, ((0xff << 16) | (1 << 24)),
				       ((pwm->pwm_pre_div << 16) | (1 << 24)));
		} else {
			prvPwmRegWrite((uint32_t)&reg->miscr,
				       ((0x3 << 6) | (0x7f << 16) | (1 << 23)),
				       ((pwm->pwm_pre_div << 16) | (1 << 23)));
		}

		/*set duty */
		prvPwmRegWrite((uint32_t)&reg->dbr, 0xffffffff,
			       ((pwm->pwm_hi << 16) | (pwm->pwm_lo)));
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		break;
	}
}

static void prvMesonConfigExt(struct xPwmMesondevice *pwm)
{
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);

	switch (pwm->hwpwm) {
	case MESON_PWM_2:
		/*set div and clock enable */
		if (pwm->chip->clk_addr) {
			if (pwm->chip->channel_separated && !pwm->chip->even_channel)
				prvPwmRegWrite(pwm->chip->clk_addr, ((0xff << 16) | (1 << 24)),
				       ((pwm->pwm_pre_div << 16) | (1 << 24)));
			else
				prvPwmRegWrite(pwm->chip->clk_addr, ((0xff << 0) | (1 << 8)),
				       ((pwm->pwm_pre_div) | (1 << 8)));
		} else {
			prvPwmRegWrite((uint32_t)&reg->miscr,
				       ((0x3 << 4) | (0x7f << 8) | (1 << 15)),
				       ((pwm->pwm_pre_div << 8) | (1 << 15)));
		}
		/*set duty */
		prvPwmRegWrite((uint32_t)&reg->da2r, 0xffffffff,
			       ((pwm->pwm_hi << 16) | (pwm->pwm_lo)));
		break;

	case MESON_PWM_3:
		/*set div and clock enable */
		if (pwm->chip->clk_addr)
			prvPwmRegWrite(pwm->chip->clk_addr, ((0xff << 16) | (1 << 24)),
				       ((pwm->pwm_pre_div << 16) | (1 << 24)));
		else
			prvPwmRegWrite((uint32_t)&reg->miscr,
				       ((0x3 << 6) | (0x7f << 16) | (1 << 23)),
				       ((pwm->pwm_pre_div << 16) | (1 << 23)));
		/*set duty */
		prvPwmRegWrite((uint32_t)&reg->db2r, 0xffffffff,
			       ((pwm->pwm_hi << 16) | (pwm->pwm_lo)));
		break;
	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		break;
	}
}

int32_t xPwmMesonConfig(struct xPwmMesondevice *pwm, uint32_t duty_ns, uint32_t period_ns)
{
	int32_t tmp;

	if ((duty_ns > period_ns) || (!period_ns)) {
		printf("Not available duty_ns period_ns !\n");
		return -1;
	}

	/* If using clktree */
	if (pwm->chip->clk_addr)
		prvPwmMesonClockSet(pwm);
	tmp = prvPwmCalc(pwm, duty_ns, period_ns);
	if (tmp != 0)
		printf("calc pwm freq err error");

	switch (pwm->hwpwm) {
	case MESON_PWM_0:
	case MESON_PWM_1:
		prvMesonConfig(pwm);
		break;

	case MESON_PWM_2:
	case MESON_PWM_3:
		prvMesonConfigExt(pwm);
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		break;
	}

	return 0;
}

void vPwmMesonDisable(struct xPwmMesondevice *pwm)
{
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);

	switch (pwm->hwpwm) {
	case MESON_PWM_0:
		prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 0), 0);
		break;

	case MESON_PWM_1:
		prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 1), 0);
		break;

	case MESON_PWM_2:
		prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 25), 0);
		break;

	case MESON_PWM_3:
		prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 24), 0);
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		break;
	}
}

/**
 * pwm_meson_enable() - enable pwm output
 * @pwm: pwm channel to choose
 */
void vPwmMesonEnable(struct xPwmMesondevice *pwm)
{
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);

	switch (pwm->hwpwm) {
	case MESON_PWM_0:
		prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 0), (1 << 0));
		break;

	case MESON_PWM_1:
		prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 1), (1 << 1));
		break;

	case MESON_PWM_2:
		prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 25), (1 << 25));
		break;

	case MESON_PWM_3:
		prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 24), (1 << 24));
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		break;
	}
}

void vPwmMesonSetTimes(struct xPwmMesondevice *pwm, uint32_t times)
{
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);

	times--;
	switch (pwm->hwpwm) {
	case MESON_PWM_0:
		prvPwmRegWrite((uint32_t)&reg->tr, (0xff << 24), (times << 24));
		break;

	case MESON_PWM_1:
		prvPwmRegWrite((uint32_t)&reg->tr, (0xff << 8), (times << 8));
		break;

	case MESON_PWM_2:
		prvPwmRegWrite((uint32_t)&reg->tr, (0xff << 16), (times << 16));
		break;

	case MESON_PWM_3:
		prvPwmRegWrite((uint32_t)&reg->tr, (0xff << 0), (times << 0));
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		break;
	}
}

void vPwmMesonSetBlinkTimes(struct xPwmMesondevice *pwm, uint32_t times)
{
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);

	times--;
	switch (pwm->hwpwm) {
	case MESON_PWM_0:
		prvPwmRegWrite((uint32_t)&reg->br, 0xf, times);
		break;

	case MESON_PWM_1:
		prvPwmRegWrite((uint32_t)&reg->br, (0xf << 4), (times << 4));
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		break;
	}
}

void vPwmMesonBlinkEnable(struct xPwmMesondevice *pwm)
{
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);

	switch (pwm->hwpwm) {
	case MESON_PWM_0:
		prvPwmRegWrite((uint32_t)&reg->br, (1 << 8), (1 << 8));
		break;

	case MESON_PWM_1:
		prvPwmRegWrite((uint32_t)&reg->br, (1 << 9), (1 << 9));
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		break;
	}
}

void vPwmMesonBlinkDisable(struct xPwmMesondevice *pwm)
{
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);

	switch (pwm->hwpwm) {
	case MESON_PWM_0:
		prvPwmRegWrite((uint32_t)&reg->br, (1 << 8), (0 << 8));
		break;

	case MESON_PWM_1:
		prvPwmRegWrite((uint32_t)&reg->br, (1 << 9), (0 << 9));
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		break;
	}
}

int32_t xPwmMesonIsBlinkComplete(struct xPwmMesondevice *pwm)
{
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);
	uint32_t a1, val;

	switch (pwm->hwpwm) {
	case MESON_PWM_0:
		val = 0x1 << 0;
		break;

	case MESON_PWM_1:
		val = 0x1 << 1;
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		return 0;
	}

	a1 = pwm_readl(&reg->miscr);
	a1 = a1 & (val);
	if (a1 == 0)
		return 1;
	else
		return 0;
}

void vPwmMesonSetPolarity(struct xPwmMesondevice *pwm, uint32_t val)
{
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);

	switch (pwm->hwpwm) {
	case MESON_PWM_0:
		if (val)
			prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 26), (1 << 26));
		else
			prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 26), 0);
		break;

	case MESON_PWM_1:
		if (val)
			prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 27), (1 << 27));
		else
			prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 27), 0);
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		break;
	}
}

void vPwmMesonClear(struct xPwmMesondevice *pwm)
{
	struct xPwmMesonRegs *reg = prvDeviceToRegs(pwm);

	switch (pwm->hwpwm) {
	case MESON_PWM_0:
		prvPwmRegWrite((uint32_t)&reg->miscr, ((0xff << 8) | (1 << 25) | (3 << 4)), 0);
		prvPwmRegWrite((uint32_t)&reg->br, ((0xf) | (1 << 8)), 0);
		break;

	case MESON_PWM_1:
		prvPwmRegWrite((uint32_t)&reg->miscr, ((0xff << 16) | (1 << 24) | (0x3 << 6)), 0);
		prvPwmRegWrite((uint32_t)&reg->br, ((0xf << 4) | (1 << 9)), 0);
		break;

	default:
		printf("%s Id:%d is invalid!\n", __func__, pwm->hwpwm);
		break;
	}
}

/**
 * xPwmMesonChannelApply() - apply pwm channel
 * @chip_id: pwm controller to choose,like PWM_AB ,PWM_CD, PWM_EF
 * PWM_GH,PWM_IJ
 * @channel_id: pwm channel to choose,like MESON_PWM_0,
 * MESON_PWM_1 MESON_PWM_2 MESON_PWM_3
 */
struct xPwmMesondevice *xPwmMesonChannelApply(uint32_t chip_id, uint32_t channel_id)
{
	struct xPwmMesondevice *pwm;
	struct xPwmMesonChip *chip;

	if (chip_id >= PWM_MUX) {
		printf("pwm chip id is invail!\n");
		return NULL;
	}

	if (channel_id > MESON_PWM_3) {
		printf("pwm channel id is invail!\n");
		return NULL;
	}

	chip = prvIdToPwmChip(chip_id);
	if (!chip) {
		printf("can not get pwm Controller!\n");
		return NULL;
	}
	if (chip->channel_separated && channel_id != MESON_PWM_0 && channel_id != MESON_PWM_2) {
		printf("It is separated pwm and channel id is invail!\n");
		return NULL;
	}
	if (chip->mask & (1 << channel_id)) {
		printf("pwm channel is applied already!\n");
		return NULL;
	}

	pwm = (struct xPwmMesondevice *)pvPortMalloc(sizeof(*pwm));
	if (!pwm) {
		printf("pwm channel malloc fail!\n");
		return NULL;
	}

	pwm->chip = chip;
	pwm->hwpwm = channel_id;
	chip->mask |= (1 << channel_id);

	return pwm;
}

/**
 * vPwmMesonChannelFree() - free pwm channel
 * @pwm: pwm channel to choose,return by pwm_channel_apply
 */
void vPwmMesonChannelFree(struct xPwmMesondevice *pwm)
{
	struct xPwmMesonChip *chip = pwm->chip;

	if (!(chip->mask & (1 << pwm->hwpwm))) {
		printf("pwm channel is free already\n");
		return;
	}

	chip->mask &= ~(1 << pwm->hwpwm);
	vPortFree(pwm);
}

int32_t vPwmMesonsetvoltage(uint32_t voltage_id, uint32_t voltage_mv)
{
	struct xPwmMesondevice *pwm;
	struct xPwmMesonVoltage *vtable;
#if PwmMesonVolt_Duty
	struct xPwmMesonRegs *reg;
#endif
	uint32_t chip_id, channel_id, duty, vtable_size, max_value, min_value;

	chip_id = prvMesonVoltToPwmchip(voltage_id);
	if (chip_id >= PWM_MUX) {
		printf("volt id:%d get chip id fail!\n", voltage_id);
		return -1;
	}

	channel_id = prvMesonVoltToPwmchannel(voltage_id);
	if (channel_id >= MESON_PWM_2) {
		printf("volt id:%d get channel id fail!\n", voltage_id);
		return -1;
	}

	vtable = vPwmMesonGetVoltTable(voltage_id);
	if (!vtable) {
		printf("volt id:%d pwm get volt table fail!\n", voltage_id);
		return -1;
	}

	vtable_size = vPwmMesonGetVoltTableSize(voltage_id);
	if (!vtable_size) {
		printf("volt id:%d pwm get volt table size fail!\n", voltage_id);
		return -1;
	}

	duty = vPwmMesonVolttoDuty(vtable, vtable_size, voltage_mv);
	if (!duty) {
		printf("volt id: %d, volt: %d get duty fail!\n", voltage_id, voltage_mv);
		return -1;
	}

	min_value = vtable[0].Voltage_mv;
	max_value = vtable[vtable_size - 1].Voltage_mv;

	pwm = xPwmMesonChannelApply(chip_id, channel_id);
	if (!pwm) {
		printf("volt id:%d pwm device apply fail!\n", voltage_id);
		return -1;
	}

#if PwmMesonVolt_Duty
	/* only update duty reg */
	reg = prvDeviceToRegs(pwm);
	if (channel_id == MESON_PWM_0) {
		prvPwmRegWrite((uint32_t)&reg->dar, 0xffffffff, duty);
		//Vddee outputs the maximum or minimum voltage, pwm outputs all high or all low, the
		//corresponding register should be cleared
		if ((voltage_mv == max_value) || (voltage_mv == min_value))
			prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 28), (1 << 28));
		else
			prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 28), (0 << 28));
	} else {
		prvPwmRegWrite((uint32_t)&reg->dbr, 0xffffffff, duty);
		if ((voltage_mv == max_value) || (voltage_mv == min_value))
			prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 29), (1 << 29));
		else
			prvPwmRegWrite((uint32_t)&reg->miscr, (1 << 29), (0 << 29));
	}
#else
	pwm->pwm_hi = duty >> 16;
	pwm->pwm_lo = duty & 0xFFFF;
	prvMesonConfig(pwm);
	vPwmMesonEnable(pwm);
#endif
	vPwmMesonChannelFree(pwm);

	return 0;
}

int32_t vPwmMesongetvoltage(uint32_t voltage_id)
{
	struct xPwmMesondevice *pwm;
	struct xPwmMesonVoltage *vtable;
	struct xPwmMesonRegs *reg;
	uint32_t chip_id, channel_id, duty, vtable_size;
	int32_t voltage_mv;

	chip_id = prvMesonVoltToPwmchip(voltage_id);
	if (chip_id >= PWM_MUX) {
		printf("volt id:%d get chip id fail!\n", voltage_id);
		return -1;
	}

	channel_id = prvMesonVoltToPwmchannel(voltage_id);
	if (channel_id >= MESON_PWM_2) {
		printf("volt id:%d get channel id fail!\n", voltage_id);
		return -1;
	}

	vtable = vPwmMesonGetVoltTable(voltage_id);
	if (!vtable) {
		printf("volt id:%d pwm get volt table fail!\n", voltage_id);
		return -1;
	}

	vtable_size = vPwmMesonGetVoltTableSize(voltage_id);
	if (!vtable_size) {
		printf("volt id:%d pwm get volt table size fail!\n", voltage_id);
		return -1;
	}

	pwm = xPwmMesonChannelApply(chip_id, channel_id);
	if (!pwm) {
		printf("volt id:%d pwm device apply fail!\n", voltage_id);
		return -1;
	}

	reg = prvDeviceToRegs(pwm);
	if (channel_id == MESON_PWM_0)
		duty = pwm_readl(&reg->dar);
	else
		duty = pwm_readl(&reg->dbr);

	vPwmMesonChannelFree(pwm);

	voltage_mv = vPwmMesonDutytoVolt(vtable, vtable_size, duty);
	if (voltage_mv < 0) {
		printf("volt id: %d, duty: 0x%x get voltage fail!\n", voltage_id, duty);
		return -1;
	}

	return voltage_mv;
}
