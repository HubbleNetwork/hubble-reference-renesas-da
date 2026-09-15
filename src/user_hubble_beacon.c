/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * A Hubble Terrestrial Network beacon on SDK6.
 *
 * There is no main(). arch_main.c owns the entry point and runs a cooperative
 * super-loop, so the usual shape of a Hubble beacon --
 *
 *     hubble_init() once, then: build payload -> advertise -> wait -> repeat
 *
 * -- has nowhere to wait and is turned inside out into callbacks. A timer
 * stops advertising; the advertising-complete callback builds the next payload
 * and starts again.
 *
 * The advertisement layout is fixed by hubble/ble.h:
 *
 *   | len  | ad type | data   | len         | ad type | data    |
 *   |------+---------+--------+-------------+---------+---------|
 *   | 0x03 | 0x03    | 0xFCA6 | out_len + 1 | 0x16    | payload |
 *
 * Both structures are required. A gateway ignores the beacon if either is
 * missing, and nothing on the device can tell you that is why.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app.h"
#include "app_default_handlers.h"
#include "app_easy_gap.h"
#include "app_easy_timer.h"
#include "arch.h"
#include "co_bt.h"
#include "compiler.h"
#include "gap.h"
#include "gapm_task.h"

#include <hubble/ble.h>
#include <hubble/hubble.h>
#include <hubble/port/crypto.h>
#include <hubble/port/sys.h>

#include "user_hubble_beacon.h"

/*
 * The device key and the provisioned wall-clock time, as master_key and
 * unix_time (milliseconds since the Unix epoch).
 *
 * Both files are written together by the Hubble SDK's embed_key_time.py. They
 * are gitignored and absent from a fresh clone -- untracked rather than
 * committed stubs, because .gitignore does not apply to files git already
 * tracks. __has_include so the failure is the #error below rather than
 * "key.c: No such file or directory".
 *
 * unix_time is the moment the image was provisioned; see hubble_config.h for
 * why that is the best wall clock this part has. For a product, provision the
 * key per-device into NVDS or OTP rather than compiling one shared key into
 * one image.
 */
#if defined(__has_include)
#if __has_include("hubble/key.c")
#include "hubble/key.c"
#endif
#if __has_include("hubble/time.c")
#include "hubble/time.c"
#endif
#endif

#if !defined(HUBBLE_KEY_SET) || !defined(HUBBLE_UNIX_TIME_SET)
#error "No device key or provisioned time. Run: python external/hubble-device-sdk/tools/embed_key_time.py <your-key-file> -o src/hubble  (see the README)"
#endif

_Static_assert(sizeof(master_key) == CONFIG_HUBBLE_KEY_SIZE,
	       "master_key length must equal CONFIG_HUBBLE_KEY_SIZE");

/* Bluetooth SIG assigned numbers for the two AD types above. */
#define AD_TYPE_COMPLETE_LIST_16_BIT_UUID 0x03U
#define AD_TYPE_SERVICE_16_BIT_DATA       0x16U

/*
 * Bytes of application payload per advertisement, 0 .. HUBBLE_BLE_MAX_DATA_LEN.
 * Zero still reports presence and location.
 */
#define HUBBLE_USER_PAYLOAD_LEN 0

/* (len + type + UUID) + (len + type) + the Hubble payload. */
#define HUBBLE_ADV_TOTAL_LEN                                                   \
	(4U + 2U + HUBBLE_BLE_ADV_HEADER_SIZE + HUBBLE_USER_PAYLOAD_LEN)

_Static_assert(HUBBLE_USER_PAYLOAD_LEN <= HUBBLE_BLE_MAX_DATA_LEN,
	       "HUBBLE_USER_PAYLOAD_LEN exceeds HUBBLE_BLE_MAX_DATA_LEN");
_Static_assert(HUBBLE_ADV_TOTAL_LEN <= ADV_DATA_LEN,
	       "Hubble advertisement does not fit in 31 bytes of AD data");

/* app_easy_timer() counts in 10 ms ticks. */
#define HUBBLE_ADV_REFRESH_TICKS (HUBBLE_ADV_REFRESH_PERIOD_S * 100U)

_Static_assert(HUBBLE_ADV_REFRESH_TICKS > 0U &&
		       HUBBLE_ADV_REFRESH_TICKS <= KE_TIMER_DELAY_MAX,
	       "HUBBLE_ADV_REFRESH_PERIOD_S outside app_easy_timer range");

/*
 * Retention memory. Currently belt and braces -- CFG_RETAIN_RAM_1_BLOCK keeps
 * every RAM block alive in extended sleep -- but it is what these would need
 * if a future config retained less. See hubble/hubble_port.c.
 */
static timer_hnd _adv_refresh_timer __SECTION_ZERO("retention_mem_area0");
static uint8_t _adv_data[ADV_DATA_LEN] __SECTION_ZERO("retention_mem_area0");
static uint8_t _adv_data_len __SECTION_ZERO("retention_mem_area0");

/*
 * Generate a fresh authenticated payload and wrap it in the two AD structures
 * a gateway requires. Returns 0, or the SDK's error -- which can be transient,
 * the nonce-reuse check rejecting a repeat being the likely one.
 */
static int _adv_data_build(void)
{
	/* Sized for the largest payload hubble_ble_advertise_get() can return. */
	uint8_t out[HUBBLE_BLE_ADV_HEADER_SIZE + HUBBLE_BLE_MAX_DATA_LEN];
	size_t payload_len = sizeof(out);
	uint8_t *p = _adv_data;
	int ret;

#if (HUBBLE_USER_PAYLOAD_LEN > 0)
	uint8_t user_payload[HUBBLE_USER_PAYLOAD_LEN] = {0};

	/* Application data goes in user_payload[] here. */

	ret = hubble_ble_advertise_get(user_payload, sizeof(user_payload), out,
				       &payload_len);
#else
	ret = hubble_ble_advertise_get(NULL, 0U, out, &payload_len);
#endif

	if (ret != 0) {
		return ret;
	}

	/* Complete list of 16-bit service UUIDs, holding 0xFCA6. */
	*p++ = 1U + sizeof(uint16_t);
	*p++ = AD_TYPE_COMPLETE_LIST_16_BIT_UUID;
	*p++ = (uint8_t)(HUBBLE_BLE_UUID & 0xFFU);
	*p++ = (uint8_t)((HUBBLE_BLE_UUID >> 8) & 0xFFU);

	/*
	 * Service data. The UUID is already the first two bytes of `out`, so
	 * `payload_len + 1` covers only the AD type byte.
	 */
	*p++ = (uint8_t)(payload_len + 1U);
	*p++ = AD_TYPE_SERVICE_16_BIT_DATA;
	memcpy(p, out, payload_len);
	p += payload_len;

	_adv_data_len = (uint8_t)(p - _adv_data);

	return 0;
}

/*
 * Refresh expiry: stop advertising. The restart happens in
 * user_app_adv_nonconn_complete() once the stack confirms the run has ended;
 * rebuilding here would race the advertising command still in flight.
 */
static void _adv_refresh_timer_cb(void)
{
	_adv_refresh_timer = EASY_TIMER_INVALID_TIMER;
	app_easy_gap_advertise_stop();
}

/* Retry after a failed build. Separate because nothing was started to stop. */
static void _adv_retry_timer_cb(void)
{
	_adv_refresh_timer = EASY_TIMER_INVALID_TIMER;
	user_app_adv_start();
}

void user_app_init(void)
{
	default_app_on_init();

	/*
	 * hubble_init() reads hubble_uptime_get() to work out the offset
	 * between unix_time and the clock, so the RTC has to be running by
	 * now. It is: periph_init() started it, well before app_on_init.
	 *
	 * Safe before the stack is configured -- hubble_init() touches no
	 * hardware. On failure there is no payload to advertise and nothing
	 * useful to do; the super-loop carries on so the part stays
	 * debuggable.
	 */
	(void)hubble_init(unix_time, master_key);
}

void user_app_adv_start(void)
{
	struct gapm_start_advertise_cmd *cmd;

	if (_adv_data_build() != 0) {
		/*
		 * Re-arm rather than return: without this one transient failure
		 * silences the beacon permanently, which looks identical to a
		 * flat battery.
		 */
		if (_adv_refresh_timer == EASY_TIMER_INVALID_TIMER) {
			_adv_refresh_timer = app_easy_timer(
				HUBBLE_ADV_REFRESH_TICKS, _adv_retry_timer_cb);
		}
		return;
	}

	/*
	 * Non-connectable. Besides being what a beacon is, this stops the stack
	 * prepending the 3-byte Flags structure, leaving all 31 bytes for us.
	 */
	cmd = app_easy_gap_non_connectable_advertise_get_active();

	cmd->info.host.adv_data_len = _adv_data_len;
	memcpy(cmd->info.host.adv_data, _adv_data, _adv_data_len);
	cmd->info.host.scan_rsp_data_len = 0U;

	app_easy_gap_non_connectable_advertise_start();

	if (_adv_refresh_timer == EASY_TIMER_INVALID_TIMER) {
		_adv_refresh_timer = app_easy_timer(HUBBLE_ADV_REFRESH_TICKS,
						    _adv_refresh_timer_cb);
	}
}

void user_app_adv_nonconn_complete(uint8_t status)
{
	/*
	 * Restarted on any status. In practice a run only ends here because the
	 * refresh timer stopped it (GAP_ERR_CANCELED); filtering on that would
	 * turn any future status into a beacon that stops for good.
	 */
	(void)status;

	user_app_adv_start();
}

void user_catch_rest_hndl(ke_msg_id_t const msgid, void const *param,
			  ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
	/* Nothing to do: the beacon defines no kernel messages of its own. */
	(void)msgid;
	(void)param;
	(void)dest_id;
	(void)src_id;
}
