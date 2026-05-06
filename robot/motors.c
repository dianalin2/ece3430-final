#include "motors.h"

Timer_ClockConfig PWMTimerClockConfig = {
    .clockSel = GPTIMER_CLKSEL_BUSCLK_SEL_ENABLE,
    .divideRatio = GPTIMER_CLKDIV_RATIO_DIV_BY_1,
    .prescale = PWM_PRESCALE};

Timer_TimerConfig PWMTimerConfig = {
    .period = 0,
    .timerMode = (GPTIMER_CTRCTL_CM_DOWN | GPTIMER_CTRCTL_REPEAT_REPEAT_1)};

Timer_PWMConfig RightPWMTimerPWMConfig = {
    .index = PB19INDEX,
    .iomuxMode = (IOMUX_PINCM_PC_CONNECTED | IOMUX_MODE4),
    .ccr = 1,
    .ccpd = GPTIMER_CCPD_C0CCP1_OUTPUT,
    .ccctl = GPTIMER_CCCTL_01_COC_COMPARE,
    .octl = GPTIMER_OCTL_01_CCPO_FUNCVAL,
    .ccact = (GPTIMER_CCACT_01_CDACT_CCP_LOW | GPTIMER_CCACT_01_LACT_CCP_HIGH),
    .duty = 0,
};

Timer_PWMConfig LeftPWMTimerPWMConfig = {
    .index = PA18INDEX,
    .iomuxMode = (IOMUX_PINCM_PC_CONNECTED | IOMUX_MODE6),
    .ccr = 1,
    .ccpd = GPTIMER_CCPD_C0CCP1_OUTPUT,
    .ccctl = GPTIMER_CCCTL_01_COC_COMPARE,
    .octl = GPTIMER_OCTL_01_CCPO_FUNCVAL,
    .ccact = (GPTIMER_CCACT_01_CDACT_CCP_LOW | GPTIMER_CCACT_01_LACT_CCP_HIGH),
    .duty = 0,
};

void initializeMotors() {
  IOMUX->SECCFG.PINCM[LEFT_MOTOR_AIN1_INDEX] =
      IOMUX_MODE1 | IOMUX_PINCM_PC_CONNECTED;
  IOMUX->SECCFG.PINCM[LEFT_MOTOR_AIN2_INDEX] =
      IOMUX_MODE1 | IOMUX_PINCM_PC_CONNECTED;
  LEFT_MOTOR_PWM_PORT->DOE31_0 |= LEFT_MOTOR_PWM_MASK;
  LEFT_MOTOR_AIN1_PORT->DOE31_0 |= LEFT_MOTOR_AIN1_MASK;
  LEFT_MOTOR_AIN2_PORT->DOE31_0 |= LEFT_MOTOR_AIN2_MASK;

  IOMUX->SECCFG.PINCM[RIGHT_MOTOR_AIN1_INDEX] =
      IOMUX_MODE1 | IOMUX_PINCM_PC_CONNECTED;
  IOMUX->SECCFG.PINCM[RIGHT_MOTOR_AIN2_INDEX] =
      IOMUX_MODE1 | IOMUX_PINCM_PC_CONNECTED;
  RIGHT_MOTOR_PWM_PORT->DOE31_0 |= RIGHT_MOTOR_PWM_MASK;
  RIGHT_MOTOR_AIN1_PORT->DOE31_0 |= RIGHT_MOTOR_AIN1_MASK;
  RIGHT_MOTOR_AIN2_PORT->DOE31_0 |= RIGHT_MOTOR_AIN2_MASK;

  // Configure timers to generate the PWM output.
  InitializeTimerClock(LEFT_PWM_TIMER, &PWMTimerClockConfig);
  InitializeTimerClock(RIGHT_PWM_TIMER, &PWMTimerClockConfig);
  uint32_t ScalingFactor = (PWMTimerClockConfig.divideRatio + 1) *
                           (PWMTimerClockConfig.prescale + 1);
  PWMTimerConfig.period = PWM_PERIOD / ScalingFactor;

  InitializeTimerCompare(LEFT_PWM_TIMER, &PWMTimerConfig);
  InitializeTimerPWM(LEFT_PWM_TIMER, &LeftPWMTimerPWMConfig);
  EnableTimer(LEFT_PWM_TIMER);

  InitializeTimerCompare(RIGHT_PWM_TIMER, &PWMTimerConfig);
  InitializeTimerPWM(RIGHT_PWM_TIMER, &RightPWMTimerPWMConfig);
  EnableTimer(RIGHT_PWM_TIMER);
}

void driveMotor(GPTIMER_Regs *pwmTimer, Timer_PWMConfig *PWMTimerPWMConfig,
                GPIO_Regs *ain1Port, uint32_t ain1Mask, GPIO_Regs *ain2Port,
                uint32_t ain2Mask, uint8_t vel, int dir) {
  uint32_t ScalingFactor = (PWMTimerClockConfig.divideRatio + 1) *
                           (PWMTimerClockConfig.prescale + 1);

  PWMTimerPWMConfig->duty =
      PWMTimerConfig.period - (vel * PWM_DELTA / ScalingFactor);
  UpdateDutyCycle(pwmTimer, PWMTimerPWMConfig);

  if (dir > 0) {
    ain1Port->DOUT31_0 &= ~ain1Mask;
    ain2Port->DOUT31_0 |= ain2Mask;
  } else if (dir < 0) {
    ain1Port->DOUT31_0 |= ain1Mask;
    ain2Port->DOUT31_0 &= ~ain2Mask;
  }
}