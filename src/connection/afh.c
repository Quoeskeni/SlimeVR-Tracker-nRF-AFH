#include "afh.h"

#include <string.h>

#include <zephyr/sys/crc.h>

#define AFH_INITIAL_SCORE 0
#define AFH_SUCCESS_SCORE_STEP 2
#define AFH_FAILURE_SCORE_STEP 8
#define AFH_RSSI_GOOD_THRESHOLD (-75)
#define AFH_RSSI_WEAK_THRESHOLD (-90)
#define AFH_RSSI_GOOD_BONUS 1
#define AFH_RSSI_WEAK_PENALTY 2
#define AFH_SCORE_MIN (-127)
#define AFH_SCORE_MAX 127

static int8_t channel_scores[AFH_CHANNEL_COUNT];
static uint8_t current_channel = AFH_DEFAULT_CHANNEL;
static uint8_t current_epoch;
static uint8_t next_candidate_channel = AFH_DEFAULT_CHANNEL;

static int8_t clamp_score(int16_t score)
{
	if (score > AFH_SCORE_MAX)
		return AFH_SCORE_MAX;
	if (score < AFH_SCORE_MIN)
		return AFH_SCORE_MIN;
	return (int8_t)score;
}

void afh_init(void)
{
	memset(channel_scores, AFH_INITIAL_SCORE, sizeof(channel_scores));
	current_channel = AFH_DEFAULT_CHANNEL;
	current_epoch = 0;
	next_candidate_channel = AFH_DEFAULT_CHANNEL;
}

bool afh_is_channel_valid(uint8_t channel)
{
	return channel >= AFH_MIN_CHANNEL && channel <= AFH_MAX_CHANNEL;
}

uint8_t afh_get_channel(void)
{
	return current_channel;
}

void afh_set_channel(uint8_t channel)
{
	if (afh_is_channel_valid(channel))
		current_channel = channel;
}

uint8_t afh_get_epoch(void)
{
	return current_epoch;
}

void afh_set_epoch(uint8_t epoch)
{
	current_epoch = epoch;
}

uint8_t afh_next_epoch(void)
{
	current_epoch++;
	return current_epoch;
}

void afh_record_tx_success(uint8_t channel)
{
	if (!afh_is_channel_valid(channel))
		return;
	channel_scores[channel] = clamp_score(channel_scores[channel] + AFH_SUCCESS_SCORE_STEP);
}

void afh_record_tx_failure(uint8_t channel)
{
	if (!afh_is_channel_valid(channel))
		return;
	channel_scores[channel] = clamp_score(channel_scores[channel] - AFH_FAILURE_SCORE_STEP);
}

void afh_record_rx_packet(uint8_t channel, int8_t rssi)
{
	if (!afh_is_channel_valid(channel))
		return;

	if (rssi >= AFH_RSSI_GOOD_THRESHOLD)
		channel_scores[channel] = clamp_score(channel_scores[channel] + AFH_RSSI_GOOD_BONUS);
	else if (rssi <= AFH_RSSI_WEAK_THRESHOLD)
		channel_scores[channel] = clamp_score(channel_scores[channel] - AFH_RSSI_WEAK_PENALTY);
}

uint8_t afh_select_best_channel(void)
{
	uint8_t best_channel = current_channel;
	int8_t best_score = channel_scores[current_channel];

	/*
	 * Walk the channel map in a round-robin order instead of always starting at
	 * channel 0. This keeps automatic recovery deterministic while preventing
	 * repeated ties from pinning all trackers to the same replacement channel.
	 */
	for (uint8_t offset = 1U; offset <= AFH_CHANNEL_COUNT; offset++) {
		uint8_t channel = (uint8_t)((next_candidate_channel + offset) % AFH_CHANNEL_COUNT);

		if (!afh_is_channel_valid(channel) || channel == current_channel)
			continue;

		if (channel_scores[channel] > best_score) {
			best_score = channel_scores[channel];
			best_channel = channel;
		}
	}

	if (best_channel != current_channel)
		next_candidate_channel = best_channel;

	return best_channel;
}

static bool afh_parse_packet(const uint8_t *data, uint8_t length, uint8_t type,
			     uint8_t *tracker_id, uint8_t *channel, uint8_t *epoch)
{
	uint32_t expected_crc;
	uint32_t received_crc;

	if (data == NULL || tracker_id == NULL || channel == NULL || epoch == NULL)
		return false;
	if (length != AFH_SYNC_PACKET_SIZE || data[0] != type)
		return false;
	if (!afh_is_channel_valid(data[2]))
		return false;

	expected_crc = crc32_k_4_2_update(AFH_SYNC_CRC_SEED, data, 4);
	memcpy(&received_crc, &data[4], sizeof(received_crc));
	if (received_crc != expected_crc)
		return false;

	*tracker_id = data[1];
	*channel = data[2];
	*epoch = data[3];
	return true;
}

bool afh_parse_sync_packet(const uint8_t *data, uint8_t length,
			   uint8_t *tracker_id, uint8_t *channel, uint8_t *epoch)
{
	return afh_parse_packet(data, length, AFH_SYNC_PACKET_TYPE, tracker_id, channel, epoch);
}

bool afh_parse_ack_packet(const uint8_t *data, uint8_t length,
			  uint8_t *tracker_id, uint8_t *channel, uint8_t *epoch)
{
	return afh_parse_packet(data, length, AFH_ACK_PACKET_TYPE, tracker_id, channel, epoch);
}

static void afh_build_packet(uint8_t *data, uint8_t type, uint8_t tracker_id,
			     uint8_t channel, uint8_t epoch)
{
	uint32_t crc;

	if (data == NULL)
		return;

	data[0] = type;
	data[1] = tracker_id;
	data[2] = afh_is_channel_valid(channel) ? channel : AFH_DEFAULT_CHANNEL;
	data[3] = epoch;
	crc = crc32_k_4_2_update(AFH_SYNC_CRC_SEED, data, 4);
	memcpy(&data[4], &crc, sizeof(crc));
}

void afh_build_sync_packet(uint8_t *data, uint8_t tracker_id,
			   uint8_t channel, uint8_t epoch)
{
	afh_build_packet(data, AFH_SYNC_PACKET_TYPE, tracker_id, channel, epoch);
}

void afh_build_ack_packet(uint8_t *data, uint8_t tracker_id,
			  uint8_t channel, uint8_t epoch)
{
	afh_build_packet(data, AFH_ACK_PACKET_TYPE, tracker_id, channel, epoch);
}
