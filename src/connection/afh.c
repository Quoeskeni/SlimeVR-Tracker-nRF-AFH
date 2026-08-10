#include "afh.h"

#define AFH_SCORE_START 100
#define AFH_SCORE_MAX 1000
#define AFH_PENALTY 10

static uint8_t current_channel = 2;
static uint16_t channel_score[AFH_CHANNEL_COUNT];

void afh_init(void)
{
	for (uint8_t i = 0; i < AFH_CHANNEL_COUNT; i++)
		channel_score[i] = AFH_SCORE_START;
}

uint8_t afh_get_channel(void)
{
	uint8_t best = current_channel;

	for (uint8_t i = 0; i < AFH_CHANNEL_COUNT; i++)
	{
		if (channel_score[i] > channel_score[best])
			best = i;
	}

	current_channel = best;
	return current_channel;
}

void afh_report_success(void)
{
	uint16_t *score = &channel_score[current_channel];
	if (*score < AFH_SCORE_MAX)
		(*score)++;
}

void afh_report_failure(void)
{
	uint16_t *score = &channel_score[current_channel];
	if (*score > AFH_PENALTY)
		*score -= AFH_PENALTY;
	else
		*score = 0;
}

void afh_force_channel(uint8_t channel)
{
	if (channel < AFH_CHANNEL_COUNT)
		current_channel = channel;
}
