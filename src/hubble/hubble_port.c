/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "compiler.h"
#include "rtc.h"

#include <hubble/port/sys.h>

/*
 * Days come from watching the RTC's hour field wrap from 23 to 0, so this has
 * to be called often enough to land in hour 23 before midnight -- once an hour
 * is sufficient. Miss it and uptime loses a day for good. The beacon refreshes
 * every HUBBLE_ADV_REFRESH_PERIOD_S.
 */
uint64_t hubble_uptime_get(void)
{
	/*
	 * Placed in retention memory deliberately. With the current config it
	 * is belt and braces -- da14535_config_advanced.h defines
	 * CFG_RETAIN_RAM_1_BLOCK, which on this part makes the SDK's
	 * DO_NOT_RETAIN_ALL_RAM_BLOCKS 0, so no RAM is powered down in
	 * extended sleep and a plain static would survive too. It stops being
	 * belt and braces the moment someone retains less, which is the norm
	 * on the DA14531: the day count would reset while the RTC kept
	 * counting, and uptime would go backwards.
	 */
	static uint32_t days __SECTION_ZERO("retention_mem_area0");
	static uint8_t last_hour __SECTION_ZERO("retention_mem_area0");
	rtc_time_t time;

	rtc_get_time_clndr(&time, NULL);

	if (time.hour < last_hour) {
		days++;
	}

	last_hour = time.hour;

	return ((uint64_t)days * 86400000ULL) +
	       ((uint64_t)time.hour * 3600000ULL) +
	       ((uint64_t)time.minute * 60000ULL) +
	       ((uint64_t)time.sec * 1000ULL) + ((uint64_t)time.hsec * 10ULL);
}

/*
 * No-ops, because nothing can contend. The one critical section in the Hubble
 * SDK is reached only from user_app_adv_start(), and all of its entry points
 * arrive as kernel messages dispatched by rwip_schedule() from the single
 * super-loop in arch_main.c. SDK6 interrupts set kernel events; they do not
 * run application handlers.
 *
 * This stops being true if a Hubble API is called from a real ISR or from a
 * sleep hook, or if hubble_sequence_counter_get() is overridden to persist the
 * counter. Note that sys.h wants a blocking recursive mutex and rules out an
 * interrupt-disabling spinlock, so GLOBAL_INT_DISABLE() is not the fix.
 */
int hubble_lock_init(void)
{
	return 0;
}

void hubble_lock(void)
{
}

void hubble_unlock(void)
{
}
