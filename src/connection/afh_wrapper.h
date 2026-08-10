#pragma once

#include <stdint.h>
#include <stdbool.h>

void afh_wrapper_init(void);
int afh_wrapper_apply_channel(uint8_t channel);
int afh_wrapper_apply_current_channel(void);
int afh_wrapper_apply_default_channel(void);
int afh_wrapper_set_channel_state(uint8_t channel, uint8_t epoch);
uint8_t afh_wrapper_get_channel(void);
uint8_t afh_wrapper_get_epoch(void);
void afh_wrapper_record_tx_success(void);
void afh_wrapper_record_tx_failure(void);
void afh_wrapper_record_rx_packet(int8_t rssi);
bool afh_wrapper_queue_best_channel_if_needed(void);
bool afh_wrapper_queue_advertised_channel(uint8_t channel, uint8_t epoch);
bool afh_wrapper_take_pending_sync(uint8_t *channel, uint8_t *epoch);
bool afh_wrapper_handle_ack_packet(const uint8_t *data, uint8_t length,
				   uint8_t expected_tracker_id);
bool afh_wrapper_take_pending_channel(uint8_t *channel, uint8_t *epoch);
bool afh_wrapper_check_handshake_timeout(void);
