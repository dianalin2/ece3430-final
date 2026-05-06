#include "bno055.h"

void initializeBNO055(I2C_Regs *i2c) {
  uint8_t buffer[3] = {BNO055_OPR_MODE_ADDR, BNO055_OPERATION_MODE_NDOF, 0};
  I2C_Send(i2c, BNO055_ADDRESS, (uint8_t *)buffer, 3);

  buffer[0] = BNO055_UNIT_SEL_ADDR;
  buffer[1] = 0x0000;
  I2C_Send(i2c, BNO055_ADDRESS, (uint8_t *)buffer, 2);
}

int16_t readBNO055(I2C_Regs *i2c, uint8_t SensorRegisterAddress) {

  // Use I2C to send the address of the register to read.
  I2C_Send(i2c, BNO055_ADDRESS, &SensorRegisterAddress, 1);

  // Then use I2C to receive the register contents as 2 bytes.
  uint8_t buffer[2] = {0, 0};
  I2C_Receive(i2c, BNO055_ADDRESS, (uint8_t *)buffer, 2);

  // Finally, assign the 2 bytes to the return value as a 16-bit unsigned
  // integer
  return (int16_t)((buffer[1] << 8) | (buffer[0]));
}