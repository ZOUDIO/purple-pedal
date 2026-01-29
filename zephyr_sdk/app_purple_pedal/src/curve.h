#ifndef APP_CURVE_H_
#define APP_CURVE_H_

#include <stdint.h>
#include <stdbool.h>

// 3 Channels, 65 Points each
// TODO: Make this configurable?
#define CURVE_POINTS 65
#define CURVE_CHANNELS 3

void curve_init(void);

void curve_set_active(bool active);
bool curve_is_active(void);

// Call this to update the data for a specific channel
// start_idx: where in the 65-point array to start writing (for chunking)
// count: how many points are in this chunk
void curve_update_data(uint8_t channel, uint8_t start_idx, uint8_t count, uint16_t *data);

// The Main Math Function
// Takes the LINEAR calibrated value and applies the curve
uint16_t curve_apply(uint8_t channel, uint16_t linear_input);

#endif /* APP_CURVE_H_ */