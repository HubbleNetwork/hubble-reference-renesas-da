/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

/*
 * GAP and radio sleep configuration.
 */

#include "app_user_config.h"
#include "app_default_handlers.h"
#include "app_adv_data.h"
#include "arch_api.h"
#include "co_bt.h"

#include "user_hubble_beacon.h"
#include "user_periph_setup.h"

/* Use private, non-resolvable address. */
#define USER_CFG_ADDRESS_MODE APP_CFG_HOST_PRIV_NRPA

#define SECONDS_TO_CENTISECONDS(s) (s) * 100U

/*
 * NRPA renewal period in centi-seconds. Set to match the packet refresh period,
 * HUBBLE_ADV_REFRESH_PERIOD_S.
 */
#define NRPA_ADDR_RENEW_CS     SECONDS_TO_CENTISECONDS(HUBBLE_ADV_REFRESH_PERIOD_S)

/* Radio and most of RAM powered down between advertising events. */
static const sleep_state_t app_default_sleep_mode = ARCH_EXT_SLEEP_ON;

static const struct advertise_configuration user_adv_conf = {
	.addr_src = APP_CFG_ADDR_SRC(USER_CFG_ADDRESS_MODE),

	.intv_min = MS_TO_BLESLOTS(1000),
	.intv_max = MS_TO_BLESLOTS(1000),

	.channel_map = ADV_ALL_CHNLS_EN,

	/* Advertise, accept neither scans nor connections. */
	.mode = GAP_BROADCASTER_MODE,
	.adv_filt_policy = ADV_ALLOW_SCAN_ANY_CON_ANY,
};

/* GAP manager for beacon. */
static const struct gapm_configuration user_gapm_conf = {
	.role = GAP_ROLE_BROADCASTER,

	.max_mtu = 23,
	.max_txoctets = 251,
	.max_txtime = 2120,

	.addr_type = APP_CFG_ADDR_TYPE(USER_CFG_ADDRESS_MODE),
	.renew_dur = NRPA_ADDR_RENEW_CS,
};

/* DEF_ADV_FOREVER: never stop advertising. */
static const struct default_handlers_configuration user_default_hnd_conf = {
	.adv_scenario = DEF_ADV_FOREVER,
	.advertise_period = 0,
	.security_request_scenario = DEF_SEC_REQ_NEVER,
};

/* Unused but required for compilation */
static const struct connection_param_configuration user_connection_param_conf = {};
static const struct central_configuration user_central_conf = {};

#define USER_DEVICE_NAME        				""
#define USER_DEVICE_NAME_LEN    				(sizeof(USER_DEVICE_NAME) - 1)
#define USER_ADVERTISE_DATA                     ""
#define USER_ADVERTISE_DATA_LEN                 (sizeof(USER_ADVERTISE_DATA) - 1)
#define USER_ADVERTISE_SCAN_RESPONSE_DATA       ""
#define USER_ADVERTISE_SCAN_RESPONSE_DATA_LEN   (sizeof(USER_ADVERTISE_SCAN_RESPONSE_DATA) - 1)

#endif /* _USER_CONFIG_H_ */
