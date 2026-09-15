/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "arch.h"
#include "arch_api.h"
#include "arch_system.h"
#include "gpio.h"
#include "rtc.h"
#include "syscntl.h"
#include "system_library.h"

#include "user_periph_setup.h"

/*
 * Defined in the SDK's arch_system.c and updated by calibrate_rcx20() on every
 * wake, but not declared in any SDK header.
 */
extern rcx_time_data_t rcx_time_data;

/*
 * The RTC that hubble_uptime_get() reads.
 *
 * periph_init() runs on every wake from extended sleep as well as at boot,
 * and that shapes this function:
 *
 *   - The PD_TIM power domain and the clock divider are written every time.
 *     Both are cheap, and the divider is computed from the RCX frequency the
 *     stack re-measures on each wake, so it tracks temperature rather than
 *     being fixed at boot.
 *   - The clock itself is started once. Restarting it on a wake would pin
 *     uptime near zero and the beacon would go on advertising a payload that
 *     never leaves its first day -- silently, because nothing else notices.
 *
 * On a cold boot periph_init() runs before rcx20_calibrate(), so rcx_freq is
 * still 0 and the divider is briefly wrong. The first wake corrects it, which
 * costs a few milliseconds of uptime at start-up and nothing afterwards.
 */
static void rtc_start_clock(void)
{
	rtc_config_t cfg = {
		.hour_clk_mode = RTC_HOUR_MODE_24H,
	};
	rtc_time_t time = {
		.hour_mode = RTC_HOUR_MODE_24H,
		.pm_flag = false,
		.hour = 0U,
		.minute = 0U,
		.sec = 0U,
		.hsec = 0U,
	};
	uint16_t lp_clk_freq = arch_clk_is_RCX20()
				       ? (uint16_t)rcx_time_data.rcx_freq
				       : 32768U;

	/*
	 * The RTC lives in PD_TIM, which is asleep by default. Until the domain
	 * is open the block ignores writes and reads back as zero, with no
	 * fault to say so. Leaving it open is also what keeps the RTC counting
	 * through extended sleep, and it costs current for the life of the
	 * device.
	 */
	SetBits16(PMU_CTRL_REG, TIM_SLEEP, 0);
	while ((GetWord16(SYS_STAT_REG) & TIM_IS_UP) != TIM_IS_UP) {
	}

	rtc_clk_config(RTC_DIV_DENOM_1000, lp_clk_freq);
	rtc_clock_enable();

	/* Reset value has both counters disabled; rtc_start() clears the bit. */
	if ((GetWord32(RTC_CONTROL_REG) & RTC_TIME_DISABLE) != 0U) {
		rtc_reset();
		rtc_init(&cfg);
		rtc_set_time_clndr(&time, NULL);
		rtc_start();
	}
}

void GPIO_reservations(void)
{
}

void set_pad_functions(void)
{
}

void periph_init(void)
{
	/* Without this the radio sits below its specified output power. */
	syscntl_dcdc_turn_on_in_boost(SYSCNTL_DCDC_LEVEL_3V0);

	/* ROM patches for this silicon revision. Must run before any peripheral. */
	patch_func();

	set_pad_functions();

	/* Release the pad latches. */
	GPIO_set_pad_latch_en(true);

	rtc_start_clock();
}
