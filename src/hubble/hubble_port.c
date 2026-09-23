/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "rtc.h"

#include <hubble/port/sys.h>

#define HSEC_TO_MSEC 10U
#define SEC_TO_MSEC 1000U
#define MIN_TO_MSEC (60U * SEC_TO_MSEC)
#define HOUR_TO_MSEC (60U * MIN_TO_MSEC)
#define DAY_TO_MSEC (24U * HOUR_TO_MSEC)

/*
 * Days elapsed since 2000-01-01, the date rtc_start_clock() starts the
 * calendar at. Uptime cannot reach 2100 inside a century, so every fourth
 * year is a leap year with no exceptions.
 */
static uint32_t _days_since_2000(const rtc_calendar_t *clndr)
{
	static const uint16_t days_before_month[12] = {
		0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,
	};
	uint32_t years = clndr->year - 2000U;
	uint32_t days = (years * 365U) + ((years + 3U) / 4U) +
			days_before_month[clndr->month - 1U] + clndr->mday - 1U;

	if (((years % 4U) == 0U) && (clndr->month > 2U)) {
		days++;
	}

	return days;
}

uint64_t hubble_uptime_get(void)
{
	rtc_calendar_t before;
	rtc_calendar_t after;
	rtc_time_t time;

	do {
		/*
		 * Reading time and calendar at once stops the RTC. To better keep it in sync, read
		 * one at a time, and confirm we didn't just read on a date boundary.
		 */
		rtc_get_time_clndr(NULL, &before);
		rtc_get_time_clndr(&time, NULL);
		rtc_get_time_clndr(NULL, &after);
	} while ((before.mday != after.mday) || (before.month != after.month) ||
		 (before.year != after.year));

	return ((uint64_t)_days_since_2000(&after) * DAY_TO_MSEC) +
	       ((uint64_t)time.hour * HOUR_TO_MSEC) +
	       ((uint64_t)time.minute * MIN_TO_MSEC) +
	       ((uint64_t)time.sec * SEC_TO_MSEC) + ((uint64_t)time.hsec * HSEC_TO_MSEC);
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
