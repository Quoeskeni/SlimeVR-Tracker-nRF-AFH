#include "afh_wrapper.h"

#include "afh.h"

/*
 * ESB driver is intentionally isolated here.
 * This wrapper is the only place where AFH talks to radio control.
 *
 * TODO: bind to real esb_set_rf_channel() after ESB API location is fixed.
 */

static uint8_t current_channel;

void afh_wrapper_init(void)
{
    afh_init();
    current_channel = afh_get_channel();
}

void afh_wrapper_apply_channel(void)
{
    current_channel = afh_get_channel();

    /*
     * Radio channel switch hook.
     * Kept empty until the exact ESB wrapper call is connected.
     */
}

void afh_wrapper_tx_result(bool success)
{
    if (success) {
        afh_report_success();
    } else {
        afh_report_failure();
    }

    current_channel = afh_get_channel();
}

uint8_t afh_wrapper_get_channel(void)
{
    return current_channel;
}
