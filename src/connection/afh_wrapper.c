#include "afh_wrapper.h"

#include <errno.h>
#include <stddef.h>

#include <esb.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include "afh.h"
#include "connection.h"

LOG_MODULE_REGISTER(afh_wrapper, LOG_LEVEL_INF);

#define AFH_TX_FAILURE_SWITCH_THRESHOLD 4U
#define AFH_ACK_TIMEOUT_MS 150

static bool initialized;
static atomic_t pending_channel_valid;
static uint8_t pending_channel;
static uint8_t pending_epoch;
static bool proposal_active;
static uint8_t proposed_channel;
static uint8_t proposed_epoch;
static int64_t proposal_started_at;
static uint8_t consecutive_tx_failures;

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
	if (!afh_is_channel_valid(channel))
		return -EINVAL;

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

bool afh_wrapper_record_tx_failure(void)
{
	afh_wrapper_init();
	if (consecutive_tx_failures < UINT8_MAX)
		consecutive_tx_failures++;
	afh_record_tx_failure(afh_get_channel());
	return consecutive_tx_failures >= AFH_TX_FAILURE_SWITCH_THRESHOLD && !proposal_active;
}

bool afh_wrapper_prepare_sync_request(uint8_t *tracker_id, uint8_t *channel, uint8_t *epoch)
{
	uint8_t best_channel;

	afh_wrapper_init();
	if (tracker_id == NULL || channel == NULL || epoch == NULL || proposal_active)
		return false;

	best_channel = afh_select_best_channel();
	if (best_channel == afh_get_channel())
		return false;

	proposal_active = true;
	proposed_channel = best_channel;
	proposed_epoch = afh_next_epoch();
	proposal_started_at = k_uptime_get();
	consecutive_tx_failures = 0;

	*tracker_id = connection_get_id();
	*channel = proposed_channel;
	*epoch = proposed_epoch;
	LOG_INF("AFH proposing RF channel %u epoch %u", proposed_channel, proposed_epoch);
	return true;
}

bool afh_wrapper_handle_ack_packet(const uint8_t *data, uint8_t length, uint8_t expected_tracker_id)
{
	uint8_t tracker_id;
	uint8_t channel;
	uint8_t epoch;

	afh_wrapper_init();
	if (!afh_parse_ack_packet(data, length, &tracker_id, &channel, &epoch))
		return false;
	if (!proposal_active || tracker_id != expected_tracker_id ||
	    channel != proposed_channel || epoch != proposed_epoch) {
		LOG_WRN("AFH ignored ACK tracker %u channel %u epoch %u", tracker_id, channel, epoch);
		return true;
	}

	pending_channel = channel;
	pending_epoch = epoch;
	atomic_set(&pending_channel_valid, 1);
	proposal_active = false;
	LOG_INF("AFH ACK queued RF channel %u epoch %u", channel, epoch);
	return true;
}

bool afh_wrapper_take_pending_channel(uint8_t *channel, uint8_t *epoch)
{
	afh_wrapper_init();
	if (channel == NULL || epoch == NULL)
		return false;
	if (!atomic_cas(&pending_channel_valid, 1, 0))
		return false;

	*channel = pending_channel;
	*epoch = pending_epoch;
	return true;
}

void afh_wrapper_check_ack_timeout(void)
{
	afh_wrapper_init();
	if (!proposal_active || k_uptime_get() - proposal_started_at < AFH_ACK_TIMEOUT_MS)
		return;

	LOG_WRN("AFH ACK timeout for channel %u epoch %u, staying on default channel", proposed_channel, proposed_epoch);
	proposal_active = false;
	if (afh_get_channel() != AFH_DEFAULT_CHANNEL) {
		pending_channel = AFH_DEFAULT_CHANNEL;
		pending_epoch = afh_next_epoch();
		atomic_set(&pending_channel_valid, 1);
	}
}
