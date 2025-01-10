////////////////////////////////////////////////////////////
//  uart header file
//  defines the uart types and constants for Raspberry Pi
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////
#pragma once

#ifndef XOS_UART_H_
#define XOS_UART_H_

#ifdef _RASPBERRY
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

// UART error codes
#define UART_OK              0
#define UART_ERROR         -1
#define UART_TIMEOUT      -2
#define UART_INVALID      -3
#define UART_NOT_OPEN     -4
#define UART_ALREADY_OPEN -5

// UART default settings
#define UART_DEFAULT_DEVICE "/dev/ttyAMA0"
#define UART_DEFAULT_BAUD   B115200
#define UART_DEFAULT_BITS   CS8
#define UART_DEFAULT_PARITY 0
#define UART_DEFAULT_STOP   0

// UART structure
typedef struct {
    int t_iHandle;              // File descriptor
    char t_cDevice[32];         // Device path
    struct termios t_tOptions;  // Terminal options
    speed_t t_iBaudRate;        // Baud rate
    int t_iBits;               // Data bits
    int t_iParity;             // Parity
    int t_iStop;               // Stop bits
    unsigned long t_ulTimeout;  // Timeout in ms
} xos_uart_t;

//////////////////////////////////
/// @brief initialize uart
/// @param p_ptUart : uart structure pointer
/// @return : success or error code
//////////////////////////////////
int uartInit(xos_uart_t* p_ptUart);

//////////////////////////////////
/// @brief open uart device
/// @param p_ptUart : uart structure pointer
/// @param p_pcDevice : device path
/// @return : success or error code
//////////////////////////////////
int uartOpen(xos_uart_t* p_ptUart, const char* p_pcDevice);

//////////////////////////////////
/// @brief configure uart
/// @param p_ptUart : uart structure pointer
/// @param p_iBaudRate : baud rate
/// @param p_iBits : data bits
/// @param p_iParity : parity
/// @param p_iStop : stop bits
/// @return : success or error code
//////////////////////////////////
int uartConfigure(xos_uart_t* p_ptUart, speed_t p_iBaudRate, int p_iBits, int p_iParity, int p_iStop);

//////////////////////////////////
/// @brief send data through uart
/// @param p_ptUart : uart structure pointer
/// @param p_pcBuffer : data buffer
/// @param p_ulSize : data size
/// @return : number of bytes sent or error code
//////////////////////////////////
int uartSend(xos_uart_t* p_ptUart, const char* p_pcBuffer, unsigned long p_ulSize);

//////////////////////////////////
/// @brief receive data from uart
/// @param p_ptUart : uart structure pointer
/// @param p_pcBuffer : data buffer
/// @param p_ulSize : buffer size
/// @return : number of bytes received or error code
//////////////////////////////////
int uartReceive(xos_uart_t* p_ptUart, char* p_pcBuffer, unsigned long p_ulSize);

//////////////////////////////////
/// @brief set uart timeout
/// @param p_ptUart : uart structure pointer
/// @param p_ulTimeout : timeout in milliseconds
/// @return : success or error code
//////////////////////////////////
int uartSetTimeout(xos_uart_t* p_ptUart, unsigned long p_ulTimeout);

//////////////////////////////////
/// @brief flush uart buffers
/// @param p_ptUart : uart structure pointer
/// @return : success or error code
//////////////////////////////////
int uartFlush(xos_uart_t* p_ptUart);

//////////////////////////////////
/// @brief close uart device
/// @param p_ptUart : uart structure pointer
/// @return : success or error code
//////////////////////////////////
int uartClose(xos_uart_t* p_ptUart);

#endif // _RASPBERRY

#endif // XOS_UART_H_


