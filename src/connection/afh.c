#include "afh.h"

#include <stddef.h>
#include <string.h>

#include <zephyr/sys/crc.h>

#define AFH_SCORE_START 100
#define AFH_SCORE_MAX 1000
#define AFH_SUCCESS_SCORE_STEP 1
#define AFH_FAILURE_SCORE_STEP 10

static uint8_t current_channel = AFH_DEFAULT_CHANNEL;
static uint8_t current_epoch;
static uint16_t channel_score[AFH_CHANNEL_COUNT];

static uint16_t clamp_score(int score)
{
	if (score < 0)
		return 0;
	if (score > AFH_SCORE_MAX)
		return AFH_SCORE_MAX;
	return (uint16_t)score;
}

void afh_init(void)
{
	current_channel = AFH_DEFAULT_CHANNEL;
	current_epoch = 0;
	for (uint8_t i = 0; i < AFH_CHANNEL_COUNT; i++)
		channel_score[i] = AFH_SCORE_START;
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
	channel_score[channel] = clamp_score(channel_score[channel] + AFH_SUCCESS_SCORE_STEP);
}

void afh_record_tx_failure(uint8_t channel)
{
	if (!afh_is_channel_valid(channel))
		return;
	channel_score[channel] = clamp_score(channel_score[channel] - AFH_FAILURE_SCORE_STEP);
}

uint8_t afh_select_best_channel(void)
{
	uint8_t best = current_channel;

	for (uint8_t i = 0; i < AFH_CHANNEL_COUNT; i++) {
		if (channel_score[i] > channel_score[best])
			best = i;
	}

	return best;
}

bool afh_parse_packet(const uint8_t *data, uint8_t length, uint8_t expected_type,
		      uint8_t *tracker_id, uint8_t *channel, uint8_t *epoch)
{
	uint32_t expected_crc;
	uint32_t received_crc;

	if (data == NULL || tracker_id == NULL || channel == NULL || epoch == NULL)
		return false;
	if (length != AFH_SYNC_PACKET_SIZE || data[0] != expected_type)
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
