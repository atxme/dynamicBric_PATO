////////////////////////////////////////////////////////////
//  spi header file
//  defines the spi types and constants for Raspberry Pi
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////
#pragma once

#ifndef XOS_SPI_H_
#define XOS_SPI_H_

#ifdef _RASPBERRY
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>

// SPI error codes
#define SPI_OK              0
#define SPI_ERROR         -1
#define SPI_TIMEOUT      -2
#define SPI_INVALID      -3
#define SPI_NOT_OPEN     -4
#define SPI_ALREADY_OPEN -5

// SPI default settings
#define SPI_DEFAULT_DEVICE "/dev/spidev0.0"
#define SPI_DEFAULT_MODE   SPI_MODE_0
#define SPI_DEFAULT_SPEED  1000000
#define SPI_DEFAULT_BITS   8
#define SPI_DEFAULT_DELAY  0

// SPI structure
typedef struct {
    int t_iHandle;              // File descriptor
    char t_cDevice[32];         // Device path
    uint8_t t_ucMode;          // SPI mode
    uint8_t t_ucBits;          // Bits per word
    uint32_t t_ulSpeed;        // Speed in Hz
    uint16_t t_usDelay;        // Delay in microseconds
    struct spi_ioc_transfer t_tTransfer;  // Transfer structure
} xos_spi_t;

//////////////////////////////////
/// @brief initialize spi
/// @param p_ptSpi : spi structure pointer
/// @return : success or error code
//////////////////////////////////
int spiInit(xos_spi_t* p_pttSpi);

//////////////////////////////////
/// @brief open spi device
/// @param p_ptSpi : spi structure pointer
/// @param p_pcDevice : device path
/// @return : success or error code
//////////////////////////////////
int spiOpen(xos_spi_t* p_pttSpi, const char* p_ptkcDevice);

//////////////////////////////////
/// @brief configure spi
/// @param p_ptSpi : spi structure pointer
/// @param p_ucMode : spi mode (0-3)
/// @param p_ulSpeed : speed in Hz
/// @param p_ucBits : bits per word
/// @param p_usDelay : delay in microseconds
/// @return : success or error code
//////////////////////////////////
int spiConfigure(xos_spi_t* p_pttSpi, uint8_t p_ucMode, uint32_t p_ulSpeed, uint8_t p_ucBits, uint16_t p_usDelay);

//////////////////////////////////
/// @brief transfer data through spi
/// @param p_ptSpi : spi structure pointer
/// @param p_pucTxBuffer : transmit buffer
/// @param p_pucRxBuffer : receive buffer
/// @param p_ulSize : buffer size
/// @return : number of bytes transferred or error code
//////////////////////////////////
int spiTransfer(xos_spi_t* p_pttSpi, const uint8_t* p_ptucTxBuffer, uint8_t* p_ptucRxBuffer, unsigned long p_ulSize);

//////////////////////////////////
/// @brief write data to spi
/// @param p_ptSpi : spi structure pointer
/// @param p_pucBuffer : data buffer
/// @param p_ulSize : buffer size
/// @return : number of bytes written or error code
//////////////////////////////////
int spiWrite(xos_spi_t* p_pttSpi, const uint8_t* p_ptucBuffer, unsigned long p_ulSize);

//////////////////////////////////
/// @brief read data from spi
/// @param p_ptSpi : spi structure pointer
/// @param p_pucBuffer : data buffer
/// @param p_ulSize : buffer size
/// @return : number of bytes read or error code
//////////////////////////////////
int spiRead(xos_spi_t* p_pttSpi, uint8_t* p_ptucBuffer, unsigned long p_ulSize);

//////////////////////////////////
/// @brief set chip select
/// @param p_ptSpi : spi structure pointer
/// @param p_bState : chip select state
/// @return : success or error code
//////////////////////////////////
int spiSetCS(xos_spi_t* p_pttSpi, int p_iState);

//////////////////////////////////
/// @brief close spi device
/// @param p_ptSpi : spi structure pointer
/// @return : success or error code
//////////////////////////////////
int spiClose(xos_spi_t* p_pttSpi);

#endif // _RASPBERRY

#endif // XOS_SPI_H_
