/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USER_HUBBLE_BEACON_H_
#define _USER_HUBBLE_BEACON_H_

/*
 * The application's SDK6 entry points. config/user_callback_config.h includes
 * this while building the static callback tables the stack reads.
 */

#include "app_entry_point.h"
#include "ke_msg.h"

/*
 * Seconds between advertisement refreshes -- how often the advertised
 * identifier changes. Not the advertising interval, which is user_adv_conf in
 * config/user_config.h.
 */
#define HUBBLE_ADV_REFRESH_PERIOD_S     (10)

void user_app_init(void);
void user_app_adv_start(void);

/* A non-connectable advertising run ended; install the next payload. */
void user_app_adv_nonconn_complete(uint8_t status);

/* Catch-all for kernel messages the SDK's dispatcher does not recognise. */
void user_catch_rest_hndl(ke_msg_id_t const msgid, void const *param,
			  ke_task_id_t const dest_id, ke_task_id_t const src_id);

#endif /* _USER_HUBBLE_BEACON_H_ */
