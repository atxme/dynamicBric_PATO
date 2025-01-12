////////////////////////////////////////////////////////////
//  event handler header file
//  defines event handler types and functions
//
// general discloser: copy or share the file is forbidden
// Written : 12/01/2025
////////////////////////////////////////////////////////////
#pragma once

#ifndef XOS_EVENT_HANDLER_H_
#define XOS_EVENT_HANDLER_H_

#include "xEvent.h"
#include <stdint.h>
#include <stdbool.h>

// Handler error codes
#define XOS_EVENT_HANDLER_OK               0
#define XOS_EVENT_HANDLER_ERROR           -1
#define XOS_EVENT_HANDLER_INVALID         -2
#define XOS_EVENT_HANDLER_FULL            -3
#define XOS_EVENT_HANDLER_NOT_FOUND       -4
#define XOS_EVENT_HANDLER_ALREADY_EXISTS  -5

// Handler limits
#define XOS_EVENT_HANDLER_MAX_SUBSCRIBERS  32
#define XOS_EVENT_HANDLER_MAX_EVENTS       64

// Subscriber structure
typedef struct {
    uint32_t t_ulEventId;                  // Event identifier
    xos_event_callback_t t_pfCallback;     // Callback function
    void* t_ptArg;                         // Callback argument
    uint8_t t_ucFlags;                     // Subscriber flags
    bool t_bActive;                        // Active status
} xos_event_subscriber_t;

//////////////////////////////////
/// @brief Initialize event handler
/// @return success or error code
//////////////////////////////////
int xEventHandlerInit(void);

//////////////////////////////////
/// @brief Register event subscriber
/// @param p_ulEventId : event identifier
/// @param p_pfCallback : callback function
/// @param p_ptArg : callback argument
/// @param p_ucFlags : subscriber flags
/// @return success or error code
//////////////////////////////////
int xEventHandlerSubscribe(uint32_t p_ulEventId,
    xos_event_callback_t p_pfCallback,
    void* p_ptArg,
    uint8_t p_ucFlags);

//////////////////////////////////
/// @brief Unregister event subscriber
/// @param p_ulEventId : event identifier
/// @param p_pfCallback : callback function
/// @return success or error code
//////////////////////////////////
int xEventHandlerUnsubscribe(uint32_t p_ulEventId,
    xos_event_callback_t p_pfCallback);

//////////////////////////////////
/// @brief Process event callbacks
/// @param p_ptEvent : event to process
/// @return success or error code
//////////////////////////////////
int xEventHandlerProcess(xos_event_t* p_ptEvent);

//////////////////////////////////
/// @brief Clean up event handler
/// @return success or error code
//////////////////////////////////
int xEventHandlerCleanup(void);

#endif // XOS_EVENT_HANDLER_H_
