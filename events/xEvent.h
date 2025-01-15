////////////////////////////////////////////////////////////
//  event header file
//  defines event types and functions
//
// general discloser: copy or share the file is forbidden
// Written : 12/01/2025
////////////////////////////////////////////////////////////
#pragma once

#ifndef XOS_EVENT_H_
#define XOS_EVENT_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Event system limits
#define XOS_EVENT_MAX_EVENTS    64


// Event error codes
#define XOS_EVENT_OK              0    // Success
#define XOS_EVENT_ERROR          -1    // Generic error
#define XOS_EVENT_INVALID        -2    // Invalid parameter
#define XOS_EVENT_TIMEOUT        -3    // Operation timeout
#define XOS_EVENT_FULL           -4    // Queue is full
#define XOS_EVENT_NOT_FOUND      -5    // Event not found
#define XOS_EVENT_ALREADY_INIT   -6    // Already initialized
#define XOS_EVENT_OVERFLOW       -7    // Queue overflow
#define XOS_EVENT_UNDERFLOW      -8    // Queue underflow

// Event priorities
typedef enum {
    XOS_EVENT_PRIORITY_LOW = 0,    // Low priority events
    XOS_EVENT_PRIORITY_MEDIUM = 1,    // Medium priority events
    XOS_EVENT_PRIORITY_HIGH = 2,    // High priority events
    XOS_EVENT_PRIORITY_URGENT = 3,    // Urgent priority events
    XOS_EVENT_PRIORITY_SYSTEM = 4     // System priority events
} xos_event_priority_t;

// Event types
typedef enum {
    XOS_EVENT_TYPE_SYSTEM = 0,    // System events (core functions)
    XOS_EVENT_TYPE_USER = 1,    // User events (application specific)
    XOS_EVENT_TYPE_ERROR = 2,    // Error events (system errors)
    XOS_EVENT_TYPE_DEBUG = 3,    // Debug events (debugging info)
    XOS_EVENT_TYPE_TIMER = 4,    // Timer events (timing related)
    XOS_EVENT_TYPE_IO = 5     // IO events (input/output operations)
} xos_event_type_t;

// Event flags
#define XOS_EVENT_FLAG_NONE          0x00    // No flags
#define XOS_EVENT_FLAG_PERSISTENT    0x01    // Event persists after processing
#define XOS_EVENT_FLAG_BROADCAST     0x02    // Event sent to all subscribers
#define XOS_EVENT_FLAG_QUEUED        0x04    // Event is queued if busy
#define XOS_EVENT_FLAG_ASYNC         0x08    // Event processed asynchronously
#define XOS_EVENT_FLAG_PERIODIC      0x10    // Event occurs periodically
#define XOS_EVENT_FLAG_PRIORITY      0x12    // Event priority is enforced

// Event statistics structure
typedef struct {
    uint32_t t_ulTotalEvents;      // Total events processed
    uint32_t t_ulDroppedEvents;    // Events dropped due to overflow
    uint32_t t_ulPeakUsage;        // Peak event queue usage
    uint32_t t_ulProcessingTime;   // Average processing time in microseconds
} xos_event_stats_t;

// Event structure
typedef struct {
    uint32_t t_ulEventId;           // Event identifier
    xos_event_type_t t_eType;       // Event type
    xos_event_priority_t t_tPrio;   // Event priority
    uint8_t t_ucFlags;              // Event flags
    void* t_ptData;                 // Event data pointer
    size_t t_ulDataSize;            // Event data size
    uint32_t t_ulTimestamp;         // Event timestamp
    uint32_t t_ulSequence;          // Event sequence number
    uint32_t t_ulTimeout;           // Event timeout in milliseconds
} xos_event_t;

// Event callback type
typedef void (*xos_event_callback_t)(xos_event_t* p_ptEvent, void* p_ptArg);

// Event filter type
typedef bool (*xos_event_filter_t)(xos_event_t* p_ptEvent, void* p_ptArg);

//////////////////////////////////
/// @brief Initialize event system
/// @return success or error code
//////////////////////////////////
int xEventInit(void);

//////////////////////////////////
/// @brief Subscribe to event
/// @param p_ulEventId : event identifier
/// @param p_pfCallback : callback function
/// @param p_ptArg : callback argument
/// @return success or error code
//////////////////////////////////
int xEventSubscribe(uint32_t p_ulEventId, xos_event_callback_t p_pfCallback, void* p_ptArg);

//////////////////////////////////
/// @brief Unsubscribe from event
/// @param p_ulEventId : event identifier
/// @param p_pfCallback : callback function
/// @return success or error code
//////////////////////////////////
int xEventUnsubscribe(uint32_t p_ulEventId, xos_event_callback_t p_pfCallback);

//////////////////////////////////
/// @brief Publish event
/// @param p_ulEventId : event identifier
/// @param p_eType : event type
/// @param p_tPrio : event priority
/// @param p_ucFlags : event flags
/// @param p_ptData : event data
/// @param p_ulSize : data size
/// @return success or error code
//////////////////////////////////
int xEventPublish(uint32_t p_ulEventId, xos_event_type_t p_eType,
    xos_event_priority_t p_tPrio, uint8_t p_ucFlags,
    void* p_ptData, size_t p_ulSize);

//////////////////////////////////
/// @brief Process pending events
/// @return success or error code
//////////////////////////////////
int xEventProcess(void);

#endif // XOS_EVENT_H_
