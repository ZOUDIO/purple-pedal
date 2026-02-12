#ifndef APP_CURVE_H_
#define APP_CURVE_H_

#include <stdint.h>
#include <stdbool.h>
#include "common.h"

// The Main Math Function
// Takes the LINEAR calibrated value and applies the curve
uint16_t curve_apply(uint8_t channel, uint16_t linear_input);

#endif /* APP_CURVE_H_ */