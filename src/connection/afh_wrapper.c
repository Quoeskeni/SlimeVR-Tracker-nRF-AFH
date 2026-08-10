#include "afh_wrapper.h"

#include <errno.h>
#include <stdint.h>
#include <stddef.h>

#include <esb.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include "afh.h"

LOG_MODULE_REGISTER(afh_wrapper, LOG_LEVEL_INF);

#define AFH_TX_FAILURE_SWITCH_THRESHOLD 4U
#define AFH_HANDSHAKE_TIMEOUT_MS 150

static bool initialized;
static atomic_t pending_sync_valid;
static atomic_t pending_channel_valid;
static uint8_t pending_sync_channel;
static uint8_t pending_sync_epoch;
static uint8_t pending_apply_channel;
static uint8_t pending_apply_epoch;
static uint8_t consecutive_tx_failures;
static bool awaiting_ack;
static uint8_t awaiting_channel;
static uint8_t awaiting_epoch;
static int64_t handshake_deadline_ms;

void afh_wrapper_init(void)
{
	if (initialized)
		return;
	afh_init();
	initialized = true;
}

int afh_wrapper_apply_channel(uint8_t channel)
{
	int err;

	afh_wrapper_init();
	if (!afh_is_channel_valid(channel)) {
		LOG_WRN("AFH rejected invalid RF channel %u", channel);
		return -EINVAL;
	}

	err = esb_set_rf_channel((uint32_t)channel);
	if (err) {
		LOG_ERR("AFH failed to apply RF channel %u: %d", channel, err);
		return err;
	}

	afh_set_channel(channel);
	LOG_INF("AFH RF channel set to %u", channel);
	return 0;
}

int afh_wrapper_apply_current_channel(void)
{
	afh_wrapper_init();
	return afh_wrapper_apply_channel(afh_get_channel());
}

int afh_wrapper_apply_default_channel(void)
{
	return afh_wrapper_apply_channel(AFH_DEFAULT_CHANNEL);
}

int afh_wrapper_set_channel_state(uint8_t channel, uint8_t epoch)
{
	afh_wrapper_init();
	if (!afh_is_channel_valid(channel))
		return -EINVAL;

	afh_set_channel(channel);
	afh_set_epoch(epoch);
	return 0;
}

uint8_t afh_wrapper_get_channel(void)
{
	afh_wrapper_init();
	return afh_get_channel();
}

uint8_t afh_wrapper_get_epoch(void)
{
	afh_wrapper_init();
	return afh_get_epoch();
}

void afh_wrapper_record_tx_success(void)
{
	afh_wrapper_init();
	consecutive_tx_failures = 0;
	afh_record_tx_success(afh_get_channel());
}

void afh_wrapper_record_tx_failure(void)
{
	afh_wrapper_init();
	if (consecutive_tx_failures < UINT8_MAX)
		consecutive_tx_failures++;
	afh_record_tx_failure(afh_get_channel());
	if (consecutive_tx_failures >= AFH_TX_FAILURE_SWITCH_THRESHOLD)
		afh_wrapper_queue_best_channel_if_needed();
}

void afh_wrapper_record_rx_packet(int8_t rssi)
{
	afh_wrapper_init();
	afh_record_rx_packet(afh_get_channel(), rssi);
}

bool afh_wrapper_queue_best_channel_if_needed(void)
{
	uint8_t best_channel;

	afh_wrapper_init();
	if (awaiting_ack || atomic_get(&pending_sync_valid))
		return false;

	best_channel = afh_select_best_channel();
	if (best_channel == afh_get_channel())
		return false;

	pending_sync_channel = best_channel;
	pending_sync_epoch = afh_next_epoch();
	atomic_set(&pending_sync_valid, 1);
	consecutive_tx_failures = 0;
	LOG_INF("AFH selected better RF channel %u epoch %u", pending_sync_channel, pending_sync_epoch);
	return true;
}

bool afh_wrapper_queue_advertised_channel(uint8_t channel, uint8_t epoch)
{
	afh_wrapper_init();
	if (!afh_is_channel_valid(channel))
		return false;
	if (epoch == afh_get_epoch())
		return false;

	pending_apply_channel = channel;
	pending_apply_epoch = epoch;
	atomic_set(&pending_channel_valid, 1);
	awaiting_ack = false;
	LOG_INF("AFH receiver advertised channel %u epoch %u", channel, epoch);
	return true;
}

bool afh_wrapper_take_pending_sync(uint8_t *channel, uint8_t *epoch)
{
	afh_wrapper_init();
	if (channel == NULL || epoch == NULL)
		return false;
	if (!atomic_cas(&pending_sync_valid, 1, 0))
		return false;

	*channel = pending_sync_channel;
	*epoch = pending_sync_epoch;
	awaiting_channel = pending_sync_channel;
	awaiting_epoch = pending_sync_epoch;
	awaiting_ack = true;
	handshake_deadline_ms = k_uptime_get() + AFH_HANDSHAKE_TIMEOUT_MS;
	return true;
}

bool afh_wrapper_handle_ack_packet(const uint8_t *data, uint8_t length,
				   uint8_t expected_tracker_id)
{
	uint8_t tracker_id;
	uint8_t channel;
	uint8_t epoch;

	afh_wrapper_init();
	if (!afh_parse_ack_packet(data, length, &tracker_id, &channel, &epoch))
		return false;
	if (tracker_id != expected_tracker_id) {
		LOG_WRN("AFH ACK for tracker %u ignored by tracker %u", tracker_id, expected_tracker_id);
		return true;
	}
	if (!awaiting_ack || channel != awaiting_channel || epoch != awaiting_epoch) {
		LOG_WRN("AFH ACK channel %u epoch %u does not match pending channel %u epoch %u",
			channel, epoch, awaiting_channel, awaiting_epoch);
		return true;
	}

	pending_apply_channel = channel;
	pending_apply_epoch = epoch;
	atomic_set(&pending_channel_valid, 1);
	awaiting_ack = false;
	LOG_INF("AFH ACK accepted: channel %u epoch %u", channel, epoch);
	return true;
}

bool afh_wrapper_take_pending_channel(uint8_t *channel, uint8_t *epoch)
{
	afh_wrapper_init();
	if (channel == NULL || epoch == NULL)
		return false;
	if (!atomic_cas(&pending_channel_valid, 1, 0))
		return false;

	*channel = pending_apply_channel;
	*epoch = pending_apply_epoch;
	return true;
}

bool afh_wrapper_check_handshake_timeout(void)
{
	afh_wrapper_init();
	if (!awaiting_ack || k_uptime_get() < handshake_deadline_ms)
		return false;

	awaiting_ack = false;
	pending_apply_channel = AFH_DEFAULT_CHANNEL;
	pending_apply_epoch = afh_get_epoch();
	atomic_set(&pending_channel_valid, 1);
	LOG_WRN("AFH ACK timeout, falling back to default channel %u", AFH_DEFAULT_CHANNEL);
	return true;
}
