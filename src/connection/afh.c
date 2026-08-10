#include "afh.h"

static uint8_t current_channel = 2;
static uint16_t channel_score[AFH_CHANNEL_COUNT];

void afh_init(void)
{
	for (int i = 0; i < AFH_CHANNEL_COUNT; i++)
		channel_score[i] = 100;
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
	if (channel_score[current_channel] < 1000)
		channel_score[current_channel]++;
}

void afh_report_failure(void)
{
	if (channel_score[current_channel] > 0)
		channel_score[current_channel] -= 5;
}

void afh_force_channel(uint8_t channel)
{
	if (channel < AFH_CHANNEL_COUNT)
		current_channel = channel;
}
