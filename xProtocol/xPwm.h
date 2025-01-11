////////////////////////////////////////////////////////////
//  pwm header file
//  defines the pwm types and constants for Raspberry Pi
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////
#pragma once

#ifndef XOS_PWM_H_
#define XOS_PWM_H_

#ifdef _RASPBERRY
#include <stdint.h>

// PWM error codes
#define XOS_PWM_OK              0
#define XOS_PWM_ERROR         -1
#define XOS_PWM_INVALID      -2
#define XOS_PWM_NOT_OPEN     -3
#define XOS_PWM_ALREADY_OPEN -4

// PWM channels
#define XOS_PWM_CHANNEL_0    0
#define XOS_PWM_CHANNEL_1    1

// PWM default settings
#define XOS_PWM_DEFAULT_FREQ     1000    // 1 kHz
#define XOS_PWM_DEFAULT_DUTY     0       // 0%
#define XOS_PWM_DEFAULT_PERIOD   1000000 // 1ms in ns

// PWM structure
typedef struct {
    int t_iHandle;              // File descriptor
    uint8_t t_ucChannel;        // PWM channel
    uint32_t t_ulFrequency;     // Frequency in Hz
    uint32_t t_ulDutyCycle;     // Duty cycle (0-100)
    uint32_t t_ulPeriod;        // Period in nanoseconds
    int t_iEnabled;             // PWM enabled flag
} xos_pwm_t;

//////////////////////////////////
/// @brief initialize pwm
/// @param p_pttPwm : pwm structure pointer
/// @return : success or error code
//////////////////////////////////
int pwmInit(xos_pwm_t* p_pttPwm);

//////////////////////////////////
/// @brief open pwm channel
/// @param p_pttPwm : pwm structure pointer
/// @param p_ucChannel : pwm channel
/// @return : success or error code
//////////////////////////////////
int pwmOpen(xos_pwm_t* p_pttPwm, uint8_t p_ucChannel);

//////////////////////////////////
/// @brief set pwm frequency
/// @param p_pttPwm : pwm structure pointer
/// @param p_ulFrequency : frequency in Hz
/// @return : success or error code
//////////////////////////////////
int pwmSetFrequency(xos_pwm_t* p_pttPwm, uint32_t p_ulFrequency);

//////////////////////////////////
/// @brief set pwm duty cycle
/// @param p_pttPwm : pwm structure pointer
/// @param p_ulDutyCycle : duty cycle (0-100)
/// @return : success or error code
//////////////////////////////////
int pwmSetDutyCycle(xos_pwm_t* p_pttPwm, uint32_t p_ulDutyCycle);

//////////////////////////////////
/// @brief enable pwm output
/// @param p_pttPwm : pwm structure pointer
/// @return : success or error code
//////////////////////////////////
int pwmEnable(xos_pwm_t* p_pttPwm);

//////////////////////////////////
/// @brief disable pwm output
/// @param p_pttPwm : pwm structure pointer
/// @return : success or error code
//////////////////////////////////
int pwmDisable(xos_pwm_t* p_pttPwm);

//////////////////////////////////
/// @brief close pwm channel
/// @param p_pttPwm : pwm structure pointer
/// @return : success or error code
//////////////////////////////////
int pwmClose(xos_pwm_t* p_pttPwm);

#endif // _RASPBERRY
#endif // XOS_PWM_H_
