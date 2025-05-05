////////////////////////////////////////////////////////////
//  timer header file
//  defines the timer types and functions
//
// general discloser: copy or share the file is forbidden
// Written : 12/01/2025
////////////////////////////////////////////////////////////
#pragma once

#ifndef XOS_TIMER_H_
#define XOS_TIMER_H_

#include <stdint.h>
#include <time.h>

// Timer error codes
#define XOS_TIMER_OK            0xD9A84B10
#define XOS_TIMER_ERROR         0xD9A84B11
#define XOS_TIMER_INVALID       0xD9A84B12
#define XOS_TIMER_TIMEOUT       0xD9A84B13
#define XOS_TIMER_NOT_INIT      0xD9A84B14

// Timer modes
#define XOS_TIMER_MODE_ONESHOT  0
#define XOS_TIMER_MODE_PERIODIC 1

typedef struct
{
    uint32_t t_ulPeriod;           // Timer period in microseconds
    uint8_t t_ucMode;              // Timer mode (one-shot or periodic)
    uint8_t t_ucActive;            // Timer active flag
    struct timespec t_tStart;      // Start time
    struct timespec t_tNext;       // Next trigger time
} xos_timer_t;

//////////////////////////////////
/// @brief Create and initialize a timer
/// @param p_ptTimer : timer structure pointer
/// @param p_ulPeriod : timer period in microseconds
/// @param p_ucMode : timer mode (XOS_TIMER_MODE_ONESHOT or XOS_TIMER_MODE_PERIODIC)
/// @return success or error code
//////////////////////////////////
int xTimerCreate(xos_timer_t* p_ptTimer, uint32_t p_ulPeriod, uint8_t p_ucMode);

//////////////////////////////////
/// @brief Start a timer
/// @param p_ptTimer : timer structure pointer
/// @return success or error code
//////////////////////////////////
int xTimerStart(xos_timer_t* p_ptTimer);

//////////////////////////////////
/// @brief Stop a timer
/// @param p_ptTimer : timer structure pointer
/// @return success or error code
//////////////////////////////////
int xTimerStop(xos_timer_t* p_ptTimer);

//////////////////////////////////
/// @brief Check if timer has expired
/// @param p_ptTimer : timer structure pointer
/// @return XOS_TIMER_OK if expired, XOS_TIMER_TIMEOUT if not expired
//////////////////////////////////
int xTimerExpired(xos_timer_t* p_ptTimer);

//////////////////////////////////
/// @brief get current time in milliseconds
/// @return current time in milliseconds
//////////////////////////////////
uint32_t xTimerGetCurrentMs(void);

//////////////////////////////////
/// @brief delay 
/// @param p_ulDelay : delay in milliseconds
//////////////////////////////////
void xTimerDelay(uint32_t p_ulDelay);

#endif // XOS_TIMER_H_
