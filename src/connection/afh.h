#ifndef SLIMENRF_AFH
#define SLIMENRF_AFH

#include <stdint.h>
#include <stdbool.h>

#define AFH_CHANNEL_COUNT 80

void afh_init(void);
uint8_t afh_get_channel(void);
void afh_report_success(void);
void afh_report_failure(void);
void afh_force_channel(uint8_t channel);

#endif
