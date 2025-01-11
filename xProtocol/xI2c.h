////////////////////////////////////////////////////////////
//  i2c header file
//  defines the i2c types and constants for Raspberry Pi
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////
#pragma once

#ifndef XOS_I2C_H_
#define XOS_I2C_H_

#ifdef _RASPBERRY
#include <stdint.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

// I2C error codes
#define XOS_I2C_OK              0
#define XOS_I2C_ERROR         -1
#define XOS_I2C_TIMEOUT      -2
#define XOS_I2C_INVALID      -3
#define XOS_I2C_NOT_OPEN     -4
#define XOS_I2C_ALREADY_OPEN -5
#define XOS_I2C_NO_ACK       -6

// I2C default settings
#define XOS_I2C_DEFAULT_BUS     1        // I2C-1 on most RPi models
#define XOS_I2C_DEFAULT_SPEED   100000   // 100 kHz standard mode
#define XOS_I2C_DEFAULT_TIMEOUT 1000     // 1 second timeout

// I2C structure
typedef struct {
    int t_iHandle;              // File descriptor
    char t_cDevice[32];         // Device path
    uint8_t t_ucAddress;        // Slave address
    uint32_t t_ulSpeed;         // Speed in Hz
    uint32_t t_ulTimeout;       // Timeout in ms
} xos_i2c_t;

//////////////////////////////////
/// @brief initialize i2c
/// @param p_pttI2c : i2c structure pointer
/// @return : success or error code
//////////////////////////////////
int i2cInit(xos_i2c_t* p_pttI2c);

//////////////////////////////////
/// @brief open i2c bus
/// @param p_pttI2c : i2c structure pointer
/// @param p_ucBus : i2c bus number
/// @return : success or error code
//////////////////////////////////
int i2cOpen(xos_i2c_t* p_pttI2c, uint8_t p_ucBus);

//////////////////////////////////
/// @brief set slave address
/// @param p_pttI2c : i2c structure pointer
/// @param p_ucAddress : slave address
/// @return : success or error code
//////////////////////////////////
int i2cSetSlaveAddress(xos_i2c_t* p_pttI2c, uint8_t p_ucAddress);

//////////////////////////////////
/// @brief write data to i2c device
/// @param p_pttI2c : i2c structure pointer
/// @param p_ptucBuffer : data buffer
/// @param p_ulSize : buffer size
/// @return : number of bytes written or error code
//////////////////////////////////
int i2cWrite(xos_i2c_t* p_pttI2c, const uint8_t* p_ptucBuffer, unsigned long p_ulSize);

//////////////////////////////////
/// @brief read data from i2c device
/// @param p_pttI2c : i2c structure pointer
/// @param p_ptucBuffer : data buffer
/// @param p_ulSize : buffer size
/// @return : number of bytes read or error code
//////////////////////////////////
int i2cRead(xos_i2c_t* p_pttI2c, uint8_t* p_ptucBuffer, unsigned long p_ulSize);

//////////////////////////////////
/// @brief write register to i2c device
/// @param p_pttI2c : i2c structure pointer
/// @param p_ucRegister : register address
/// @param p_ucValue : register value
/// @return : success or error code
//////////////////////////////////
int i2cWriteRegister(xos_i2c_t* p_pttI2c, uint8_t p_ucRegister, uint8_t p_ucValue);

//////////////////////////////////
/// @brief read register from i2c device
/// @param p_pttI2c : i2c structure pointer
/// @param p_ucRegister : register address
/// @param p_ptucValue : pointer to store value
/// @return : success or error code
//////////////////////////////////
int i2cReadRegister(xos_i2c_t* p_pttI2c, uint8_t p_ucRegister, uint8_t* p_ptucValue);

//////////////////////////////////
/// @brief set i2c bus speed
/// @param p_pttI2c : i2c structure pointer
/// @param p_ulSpeed : speed in Hz
/// @return : success or error code
//////////////////////////////////
int i2cSetSpeed(xos_i2c_t* p_pttI2c, uint32_t p_ulSpeed);

//////////////////////////////////
/// @brief close i2c bus
/// @param p_pttI2c : i2c structure pointer
/// @return : success or error code
//////////////////////////////////
int i2cClose(xos_i2c_t* p_pttI2c);

#endif // _RASPBERRY

#endif // XOS_I2C_H_
