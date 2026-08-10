#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * AFH ESB wrapper interface.
 * Keeps adaptive frequency hopping logic independent from Nordic ESB API.
 */

void afh_wrapper_init(void);

/**
 * Apply currently selected AFH channel to radio.
 */
void afh_wrapper_apply_channel(void);

/**
 * Notify wrapper about transmission result.
 */
void afh_wrapper_tx_result(bool success);

/**
 * Return currently selected RF channel.
 */
uint8_t afh_wrapper_get_channel(void);
