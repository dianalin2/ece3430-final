#include "hcsr04.h"

uint32_t readEcho(void) {
  return (ULTRASONIC_ECHO_PORT->DIN31_0 & ULTRASONIC_ECHO_MASK) >>
         ULTRASONIC_ECHO_PIN;
}

void turnOnTrig(void) {
  ULTRASONIC_TRIG_PORT->DOUT31_0 |= ULTRASONIC_TRIG_MASK;
}

void turnOffTrig(void) {
  ULTRASONIC_TRIG_PORT->DOUT31_0 &= ~ULTRASONIC_TRIG_MASK;
}

void initializeUltrasonic(void) {
  IOMUX->SECCFG.PINCM[ULTRASONIC_TRIG_INDEX] =
      IOMUX_MODE1 | IOMUX_PINCM_PC_CONNECTED;
  IOMUX->SECCFG.PINCM[ULTRASONIC_ECHO_INDEX] =
      IOMUX_MODE1 | IOMUX_PINCM_PC_CONNECTED | IOMUX_PINCM_INENA_ENABLE |
      IOMUX_PINCM_PIPD_ENABLE;
  ULTRASONIC_TRIG_PORT->DOE31_0 |= ULTRASONIC_TRIG_MASK;
}

float readUltrasonic(void) {
  turnOffTrig();
  ClockDelay(64); // 2 us
  turnOnTrig();
  ClockDelay(320); // 10 us
  turnOffTrig();

  while (!readEcho())
    ClockDelay(32);

  unsigned long delta = 0;
  while (readEcho()) {
    ClockDelay(32);
    delta++;
  }

  return (delta * 0.0343) / 2.0;
}