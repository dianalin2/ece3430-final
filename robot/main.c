#include "../../../LP_MSPM0G3507/bsp.h"
#include "../../../LP_MSPM0G3507/mspm0g350x_int.h"
#include "../../../LP_MSPM0G3507/uart.h"
#include "bno055.h"
#include "hcsr04.h"
#include "motors.h"
#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ti/devices/msp/msp.h>

#define BUFFER_LENGTH 64
#define COMMAND_LENGTH 3

uint32_t ClockFrequency = 32000000; // 32 MHz

extern Timer_PWMConfig RightPWMTimerPWMConfig;
extern Timer_PWMConfig LeftPWMTimerPWMConfig;

uint8_t motor_commands_buffer[1 + COMMAND_LENGTH];

// Global variable to configure UART
UART_Config UART0_Config = {.Tx_index = PA10INDEX,
                            .Tx_iomuxMode = IOMUX_MODE2,
                            .Rx_index = PA11INDEX,
                            .Rx_iomuxMode = IOMUX_MODE2,
                            .ClockFrequency = 0};

I2C_Config i2c1_config = {
    .SCL_index = PB2INDEX, .SDA_index = PB3INDEX, .ClockFrequency = 0};

int main(void) {
  uint8_t OutputBuffer[BUFFER_LENGTH];

  InitializeLaunchpad(ClockFrequency);
  initializeMotors();
  initializeUltrasonic();

  UART0_Config.ClockFrequency = ClockFrequency;
  InitializeUART(UART0, &UART0_Config);

  i2c1_config.ClockFrequency = ClockFrequency;
  InitializeI2C(I2C1, &i2c1_config);
  ClockDelay(ClockFrequency);
  initializeBNO055(I2C1);

  while (1) {
    motor_commands_buffer[0] = GetUserInput(UART0);

    if (motor_commands_buffer[0] != 0xff) {
      continue;
    }

    for (int i = 1; i < COMMAND_LENGTH + 1; i++) {
      motor_commands_buffer[i] = GetUserInput(UART0);
    }

    // // Echo character entered.
    // sprintf((char *)OutputBuffer, "You entered: %x %x %x %x %x %x\r\n",
    //         motor_commands_buffer[0], motor_commands_buffer[1],
    //         motor_commands_buffer[2], motor_commands_buffer[3],
    //         motor_commands_buffer[4], motor_commands_buffer[5]);
    // PrintOutputBuffer(UART0, OutputBuffer);

    int leftCommand = (((int)motor_commands_buffer[2]) - 127) * 100 / 127;
    driveMotor(LEFT_PWM_TIMER, &LeftPWMTimerPWMConfig, LEFT_MOTOR_AIN1_PORT,
               LEFT_MOTOR_AIN1_MASK, LEFT_MOTOR_AIN2_PORT, LEFT_MOTOR_AIN2_MASK,
               (uint8_t)abs(leftCommand), leftCommand * -1);

    int rightCommand = (((int)motor_commands_buffer[3]) - 127) * 100 / 127;
    driveMotor(RIGHT_PWM_TIMER, &RightPWMTimerPWMConfig, RIGHT_MOTOR_AIN1_PORT,
               RIGHT_MOTOR_AIN1_MASK, RIGHT_MOTOR_AIN2_PORT,
               RIGHT_MOTOR_AIN2_MASK, abs(rightCommand), rightCommand * 1);

    float distance = readUltrasonic();
    float ax = (float)readBNO055(I2C1, BNO055_LINEAR_ACCEL_DATA_X_ADDR);
    float ay = (float)readBNO055(I2C1, BNO055_LINEAR_ACCEL_DATA_Y_ADDR);
    float az = (float)readBNO055(I2C1, BNO055_LINEAR_ACCEL_DATA_Z_ADDR);
    float yaw = (float)readBNO055(I2C1, BNO055_EULER_H_ADDR) / 16.0;
    float roll = (float)readBNO055(I2C1, BNO055_EULER_R_ADDR) / 16.0;
    float pitch = (float)readBNO055(I2C1, BNO055_EULER_P_ADDR) / 16.0;

    // sprintf((char *)OutputBuffer, "%d %d %f\r\n", leftCommand, rightCommand,
    //         distance);

    OutputBuffer[0] = 0xFF;
    OutputBuffer[1] = 0x01;
    memcpy(&OutputBuffer[2], &distance, sizeof(float));
    memcpy(&OutputBuffer[6], &ax, sizeof(float));
    memcpy(&OutputBuffer[10], &ay, sizeof(float));
    memcpy(&OutputBuffer[14], &az, sizeof(float));
    memcpy(&OutputBuffer[18], &yaw, sizeof(float));
    memcpy(&OutputBuffer[22], &roll, sizeof(float));
    memcpy(&OutputBuffer[26], &pitch, sizeof(float));

    for (size_t i = 0; i < 30; i++) {
      while (UART0->STAT & UART_STAT_TXFF_MASK)
        ;

      UART0->TXDATA = OutputBuffer[i];
    }
  }

  return 0;
}