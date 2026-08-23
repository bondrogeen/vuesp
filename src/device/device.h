#ifndef DEVICE_H
#define DEVICE_H

#include <Wire.h>

#include "../libs/UnixTime.h"
#include "./gpio/gpio.h"
#include "./runner/runner.h"

#define DEVICE_COMMAND_SAVE 254
#define DEVICE_COMMAND_GET 1
#define DEVICE_COMMAND_SET 2
#define DEVICE_COMMAND_TEXT 3
#define BMS_LEN_RESPONSE 128

// ============================================================
// STATUS MASKS (decimal values)
// ============================================================

// ---- Status1 - Protection ----
#define STATUS1_CHARGE_HIGH_VOLTAGE    128   // 0x80 - Charger high voltage
#define STATUS1_SHORT_CIRCUIT          64    // 0x40 - Short circuit
#define STATUS1_DISCHARGE_OVER_CURRENT 32    // 0x20 - Discharge overcurrent
#define STATUS1_CHARGE_OVER_CURRENT    16    // 0x10 - Charge overcurrent
#define STATUS1_OVER_VOLTAGE           8     // 0x08 - Overvoltage
#define STATUS1_UNDER_VOLTAGE          4     // 0x04 - Undervoltage
#define STATUS1_CELL_UNDER_VOLTAGE     2     // 0x02 - Cell undervoltage
#define STATUS1_CELL_OVER_VOLTAGE      1     // 0x01 - Cell overvoltage

// ---- Status2 - Temperature Protection ----
#define STATUS2_ENV_LOW                64    // 0x40 - Environment low temp
#define STATUS2_ENV_HIGH               32    // 0x20 - Environment high temp
#define STATUS2_MOS_OVER_TEMP          16    // 0x10 - MOSFET overtemperature
#define STATUS2_DISCHARGE_LOW_TEMP     8     // 0x08 - Discharge low temp
#define STATUS2_CHARGE_LOW_TEMP        4     // 0x04 - Charge low temp
#define STATUS2_DISCHARGE_HIGH_TEMP    2     // 0x02 - Discharge high temp
#define STATUS2_CHARGE_HIGH_TEMP       1     // 0x01 - Charge high temp

// ---- Status3 - Additional Flags ----
#define STATUS3_AC_PRESENT             128   // 0x80 - AC power present
#define STATUS3_REVERSE                16    // 0x10 - Reverse polarity
#define STATUS3_DSG_MOS_ON             4     // 0x04 - Discharge MOSFET on
#define STATUS3_CHG_MOS_ON             2     // 0x02 - Charge MOSFET on

// ---- Status4 - Errors ----
#define STATUS4_ERROR_0x80             128   // 0x80 - Error
#define STATUS4_ERROR_0x40             64    // 0x40 - Error
#define STATUS4_ERROR_0x20             32    // 0x20 - Error
#define STATUS4_ERROR_0x10             16    // 0x10 - Error
#define STATUS4_ERROR_0x08             8     // 0x08 - Error
#define STATUS4_ERROR_0x04             4     // 0x04 - Error
#define STATUS4_ERROR_0x02             2     // 0x02 - Error
#define STATUS4_ERROR_0x01             1     // 0x01 - Error

// ---- Status5 - Voltage/Current Warnings ----
#define STATUS5_DSG_OVER_CURRENT       128   // 0x80 - Discharge overcurrent
#define STATUS5_CHG_OVER_CURRENT       64    // 0x40 - Charge overcurrent
#define STATUS5_OVER_VOLTAGE           32    // 0x20 - Overvoltage
#define STATUS5_UNDER_VOLTAGE          16    // 0x10 - Undervoltage
#define STATUS5_UNDER_VOLTAGE_DUP      8     // 0x08 - Undervoltage (duplicate)
#define STATUS5_CELL_OVER_DISCHARGE    4     // 0x04 - Cell overdischarge
#define STATUS5_CELL_LOW               2     // 0x02 - Cell low
#define STATUS5_CELL_HIGH              1     // 0x01 - Cell high

// ---- Status6 - Temperature Warnings ----
#define STATUS6_LOW_SOC                128   // 0x80 - Low SOC
#define STATUS6_MOS_HIGH_TEMP          64    // 0x40 - MOSFET high temp
#define STATUS6_ENV_LOW                32    // 0x20 - Environment low temp
#define STATUS6_ENV_HIGH               16    // 0x10 - Environment high temp
#define STATUS6_CHARGE_LOW_TEMP        8     // 0x08 - Charge battery low temp
#define STATUS6_DISCHARGE_LOW_TEMP     4     // 0x04 - Discharge battery low temp
#define STATUS6_DISCHARGE_HIGH_TEMP    2     // 0x02 - Discharge battery high temp
#define STATUS6_CHARGE_HIGH_TEMP       1     // 0x01 - Charge battery high temp

// ---- Status7 - Reserved ----
// (not used)

// ---- Status8 - Current Warnings ----
#define STATUS8_DSG_OVER_CURRENT       128   // 0x80 - Discharge overcurrent
#define STATUS8_CHG_OVER_CURRENT       64    // 0x40 - Charge overcurrent
#define STATUS8_OVER_VOLTAGE           32    // 0x20 - Overvoltage
#define STATUS8_UNDER_VOLTAGE          16    // 0x10 - Undervoltage
#define STATUS8_UNDER_VOLTAGE_DUP      8     // 0x08 - Undervoltage (duplicate)
#define STATUS8_CELL_OVER_DISCHARGE    4     // 0x04 - Cell overdischarge
#define STATUS8_CELL_LOW               2     // 0x02 - Cell low
#define STATUS8_CELL_HIGH              1     // 0x01 - Cell high

// ---- Status9 - Additional Warnings ----
#define STATUS9_LOW_SOC                128   // 0x80 - Low SOC
#define STATUS9_MOS_HIGH_TEMP          64    // 0x40 - MOSFET high temp
#define STATUS9_ENV_LOW                32    // 0x20 - Environment low temp
#define STATUS9_ENV_HIGH               16    // 0x10 - Environment high temp
#define STATUS9_CHARGE_LOW_TEMP        8     // 0x08 - Charge battery low temp
#define STATUS9_DISCHARGE_LOW_TEMP     4     // 0x04 - Discharge battery low temp
#define STATUS9_DISCHARGE_HIGH_TEMP    2     // 0x02 - Discharge battery high temp
#define STATUS9_CHARGE_HIGH_TEMP       1     // 0x01 - Charge battery high temp


void setupDevice();
void setupFirstDevice();
void deviceGPIO(Port* port, uint8_t type);
void loopDevice(uint32_t now);
void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task);

#endif