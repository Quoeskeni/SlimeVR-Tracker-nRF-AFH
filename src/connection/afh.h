#ifndef SLIMENRF_AFH
#define SLIMENRF_AFH

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/sys/util.h>

#define AFH_CHANNEL_COUNT 101U
#define AFH_DEFAULT_CHANNEL 100U
#define AFH_MIN_CHANNEL 0U
#define AFH_MAX_CHANNEL (AFH_CHANNEL_COUNT - 1U)
#define AFH_SYNC_PACKET_TYPE 0xF0U
#define AFH_ACK_PACKET_TYPE 0xF1U
#define AFH_SYNC_PACKET_SIZE 8U
#define AFH_ACK_PACKET_SIZE 8U
#define AFH_SYNC_CRC_SEED 0x93a409ebU

struct afh_sync_packet {
	uint8_t type;
	uint8_t tracker_id;
	uint8_t channel;
	uint8_t epoch;
	uint32_t crc;
} __packed;

void afh_init(void);
bool afh_is_channel_valid(uint8_t channel);
uint8_t afh_get_channel(void);
void afh_set_channel(uint8_t channel);
uint8_t afh_get_epoch(void);
void afh_set_epoch(uint8_t epoch);
uint8_t afh_next_epoch(void);
void afh_record_tx_success(uint8_t channel);
void afh_record_tx_failure(uint8_t channel);
void afh_record_rx_packet(uint8_t channel, int8_t rssi);
uint8_t afh_select_best_channel(void);
bool afh_parse_sync_packet(const uint8_t *data, uint8_t length,
			   uint8_t *tracker_id, uint8_t *channel, uint8_t *epoch);
bool afh_parse_ack_packet(const uint8_t *data, uint8_t length,
			  uint8_t *tracker_id, uint8_t *channel, uint8_t *epoch);
void afh_build_sync_packet(uint8_t *data, uint8_t tracker_id,
			   uint8_t channel, uint8_t epoch);
void afh_build_ack_packet(uint8_t *data, uint8_t tracker_id,
			  uint8_t channel, uint8_t epoch);

#endif
