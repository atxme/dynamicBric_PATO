////////////////////////////////////////////////////////////
//  event queue header file
//  defines event queue types and functions
//
// general discloser: copy or share the file is forbidden
// Written : 12/01/2025
////////////////////////////////////////////////////////////
#pragma once

#ifndef XOS_EVENT_QUEUE_H_
#define XOS_EVENT_QUEUE_H_

#include "xEvent.h"
#include <stdint.h>
#include <stdbool.h>

// Queue error codes
#define XOS_EVENT_QUEUE_OK            0
#define XOS_EVENT_QUEUE_ERROR        -1
#define XOS_EVENT_QUEUE_INVALID      -2
#define XOS_EVENT_QUEUE_FULL         -3
#define XOS_EVENT_QUEUE_EMPTY        -4
#define XOS_EVENT_QUEUE_NOT_INIT     -5

// Queue configuration
#define XOS_EVENT_QUEUE_MAX_SIZE    64

// Queue structure
typedef struct {
    xos_event_t t_tEvents[XOS_EVENT_QUEUE_MAX_SIZE];  // Event array
    uint32_t t_ulHead;                                // Queue head index
    uint32_t t_ulTail;                                // Queue tail index
    uint32_t t_ulCount;                               // Current event count
    uint32_t t_ulDropped;                             // Dropped events counter
    bool t_bInitialized;                              // Initialization flag
} xos_event_queue_t;

//////////////////////////////////
/// @brief Initialize event queue
/// @param p_ptQueue : queue structure pointer
/// @return success or error code
//////////////////////////////////
int xEventQueueInit(xos_event_queue_t* p_ptQueue);

//////////////////////////////////
/// @brief Push event to queue
/// @param p_ptQueue : queue structure pointer
/// @param p_ptEvent : event to push
/// @return success or error code
//////////////////////////////////
int xEventQueuePush(xos_event_queue_t* p_ptQueue, xos_event_t* p_ptEvent);

//////////////////////////////////
/// @brief Pop event from queue
/// @param p_ptQueue : queue structure pointer
/// @param p_ptEvent : event output buffer
/// @return success or error code
//////////////////////////////////
int xEventQueuePop(xos_event_queue_t* p_ptQueue, xos_event_t* p_ptEvent);

//////////////////////////////////
/// @brief Check if queue is empty
/// @param p_ptQueue : queue structure pointer
/// @return true if empty, false otherwise
//////////////////////////////////
bool xEventQueueIsEmpty(xos_event_queue_t* p_ptQueue);

//////////////////////////////////
/// @brief Check if queue is full
/// @param p_ptQueue : queue structure pointer
/// @return true if full, false otherwise
//////////////////////////////////
bool xEventQueueIsFull(xos_event_queue_t* p_ptQueue);

//////////////////////////////////
/// @brief Get queue statistics
/// @param p_ptQueue : queue structure pointer
/// @param p_pulCount : current event count
/// @param p_pulDropped : dropped events count
/// @return success or error code
//////////////////////////////////
int xEventQueueGetStats(xos_event_queue_t* p_ptQueue,
    uint32_t* p_pulCount,
    uint32_t* p_pulDropped);

#endif // XOS_EVENT_QUEUE_H_
