#ifndef PWM_H_
#define PWM_H_
#include "ti_msp_dl_config.h"

typedef enum { up, down } PWM_direction;

// Define PWM structure
typedef struct {
  uint32_t duty_cycle; // units = percent
  uint32_t period;     // units = clock cycles
  uint32_t delta;      // units = clock cycles
  PWM_direction direction;
} PWM_s;

#endif