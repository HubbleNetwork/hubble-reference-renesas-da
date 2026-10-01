/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USER_CALLBACK_CONFIG_H_
#define _USER_CALLBACK_CONFIG_H_

/*
 * Application callback registration.
 */

#include "app_callback.h"
#include "app_default_handlers.h"
#include "app_entry_point.h"
#include "app_prf_types.h"

#include "user_hubble_beacon.h"

/*
 * BLE events:
 *
 *   app_on_set_dev_config_complete  SDK handler; calls default_operation_adv
 *                                   below, which is where advertising starts
 *   app_on_adv_nonconn_complete     a run ended -- write the next payload
 */
static const struct app_callbacks user_app_callbacks = {
	.app_on_connection                  = NULL,
	.app_on_disconnect                  = NULL,
	.app_on_update_params_rejected      = NULL,
	.app_on_update_params_complete      = NULL,
	.app_on_set_dev_config_complete     = default_app_on_set_dev_config_complete,
	.app_on_adv_nonconn_complete        = user_app_adv_nonconn_complete,
	.app_on_adv_undirect_complete       = NULL,
	.app_on_adv_direct_complete         = NULL,
	.app_on_db_init_complete            = NULL,
	.app_on_scanning_completed          = NULL,
	.app_on_adv_report_ind              = NULL,
	.app_on_get_dev_name                = NULL,
	.app_on_get_dev_appearance          = NULL,
	.app_on_get_dev_slv_pref_params     = NULL,
	.app_on_set_dev_info                = NULL,
	.app_on_data_length_change          = NULL,
	.app_on_update_params_request       = NULL,
	.app_on_generate_static_random_addr = NULL,
	.app_on_svc_changed_cfg_ind         = NULL,
	.app_on_get_peer_features           = NULL,
};

/* Handler for unknown messages. */
#define app_process_catch_rest_cb user_catch_rest_hndl

/* Entry point to start advertising. */
static const struct default_app_operations user_default_app_operations = {
	.default_operation_adv = user_app_adv_start,
};

/* Super-loop hooks. Only app init is used. */
static const struct arch_main_loop_callbacks user_app_main_loop_callbacks = {
	.app_on_init            = user_app_init,
	.app_on_ble_powered     = NULL,
	.app_on_system_powered  = NULL,
	.app_before_sleep       = NULL,
	.app_validate_sleep     = NULL,
	.app_going_to_sleep     = NULL,
	.app_resume_from_sleep  = NULL,
};

/* No GATT profiles. */
static const struct prf_func_callbacks user_prf_funcs[] = {
	{TASK_ID_INVALID, NULL, NULL}
};

#endif /* _USER_CALLBACK_CONFIG_H_ */
