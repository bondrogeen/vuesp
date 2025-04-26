#ifndef MODBUS_H
#define MODBUS_H

#include "./init.h"

#define GPIO_485TX 27
#define GPIO_485RX 14

void setModbusSetup();
uint8_t transmitData(uint8_t* buffer, size_t size);
uint16_t getCrc16(uint8_t* data, uint16_t len);
extern Modbus modbus;

#endif