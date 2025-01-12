////////////////////////////////////////////////////////////
//  gpio header file
//  defines the gpio types and constants for Raspberry Pi
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////
#pragma once

#ifndef XOS_GPIO_H_
#define XOS_GPIO_H_

#ifdef _RASPBERRY
#include <stdint.h>
#include <gpiod.h>
#include <string.h>

// GPIO error codes
#define GPIO_OK              0
#define GPIO_ERROR         -1
#define GPIO_INVALID      -2
#define GPIO_NOT_OPEN     -3
#define GPIO_ALREADY_OPEN -4

// GPIO directions
#define GPIO_INPUT         0
#define GPIO_OUTPUT        1

// GPIO values
#define GPIO_LOW          0
#define GPIO_HIGH         1

// GPIO pull modes
#define GPIO_PULL_OFF     0
#define GPIO_PULL_UP      1
#define GPIO_PULL_DOWN    2

// GPIO structure
typedef struct {
    struct gpiod_chip* t_ptChip;     // GPIO chip
    struct gpiod_line* t_ptLine;     // GPIO line
    uint32_t t_ulPin;                // Pin number
    uint8_t t_ucDirection;           // Pin direction
    uint8_t t_ucValue;               // Pin value
    uint8_t t_ucPull;               // Pull up/down
    char t_cChipName[32];           // Chip name/path
} xos_gpio_t;

//////////////////////////////////
/// @brief initialize gpio
/// @param p_pttGpio : gpio structure pointer
/// @return : success or error code
//////////////////////////////////
int gpioInit(xos_gpio_t* p_pttGpio);

//////////////////////////////////
/// @brief open gpio pin
/// @param p_pttGpio : gpio structure pointer
/// @param p_ulPin : pin number (BCM)
/// @param p_ucDirection : pin direction
/// @return : success or error code
//////////////////////////////////
int gpioOpen(xos_gpio_t* p_pttGpio, uint32_t p_ulPin, uint8_t p_ucDirection);

//////////////////////////////////
/// @brief set gpio direction
/// @param p_pttGpio : gpio structure pointer
/// @param p_ucDirection : pin direction
/// @return : success or error code
//////////////////////////////////
int gpioSetDirection(xos_gpio_t* p_pttGpio, uint8_t p_ucDirection);

//////////////////////////////////
/// @brief set gpio value
/// @param p_pttGpio : gpio structure pointer
/// @param p_ucValue : pin value
/// @return : success or error code
//////////////////////////////////
int gpioSetValue(xos_gpio_t* p_pttGpio, uint8_t p_ucValue);

//////////////////////////////////
/// @brief get gpio value
/// @param p_pttGpio : gpio structure pointer
/// @param p_ptucValue : pointer to store value
/// @return : success or error code
//////////////////////////////////
int gpioGetValue(xos_gpio_t* p_pttGpio, uint8_t* p_ptucValue);

//////////////////////////////////
/// @brief set gpio pull mode
/// @param p_pttGpio : gpio structure pointer
/// @param p_ucPull : pull mode
/// @return : success or error code
//////////////////////////////////
int gpioSetPull(xos_gpio_t* p_pttGpio, uint8_t p_ucPull);

//////////////////////////////////
/// @brief close gpio pin
/// @param p_pttGpio : gpio structure pointer
/// @return : success or error code
//////////////////////////////////
int gpioClose(xos_gpio_t* p_pttGpio);

#endif // _RASPBERRY

#endif // XOS_GPIO_H_
