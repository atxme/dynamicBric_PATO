////////////////////////////////////////////////////////////
//  event queue source file
//  implements event queue functions
//
// general discloser: copy or share the file is forbidden
// Written : 12/01/2025
////////////////////////////////////////////////////////////

#include "xEventQueue.h"
#include "xAssert.h"
#include <string.h>

int xEventQueueInit(xos_event_queue_t* p_ptQueue)
{
    X_ASSERT(p_ptQueue != NULL);

    memset(p_ptQueue, 0, sizeof(xos_event_queue_t));
    p_ptQueue->t_bInitialized = true;

    return XOS_EVENT_QUEUE_OK;
}

int xEventQueuePush(xos_event_queue_t* p_ptQueue, xos_event_t* p_ptEvent)
{
    X_ASSERT(p_ptQueue != NULL);
    X_ASSERT(p_ptEvent != NULL);
    X_ASSERT(p_ptQueue->t_bInitialized == true);

    if (p_ptQueue->t_ulCount >= XOS_EVENT_QUEUE_MAX_SIZE)
    {
        p_ptQueue->t_ulDropped++;
        return XOS_EVENT_QUEUE_FULL;
    }

    /* Copier l’événement dans la file */
    memcpy(&p_ptQueue->t_tEvents[p_ptQueue->t_ulTail],
        p_ptEvent,
        sizeof(xos_event_t));

    p_ptQueue->t_ulTail = (p_ptQueue->t_ulTail + 1) % XOS_EVENT_QUEUE_MAX_SIZE;
    p_ptQueue->t_ulCount++;

    return XOS_EVENT_QUEUE_OK;

}

int xEventQueuePop(xos_event_queue_t* p_ptQueue, xos_event_t* p_ptEvent)
{
    X_ASSERT(p_ptQueue != NULL);
    X_ASSERT(p_ptEvent != NULL);
    X_ASSERT(p_ptQueue->t_bInitialized == true);

    if (p_ptQueue->t_ulCount == 0)
    {
        return XOS_EVENT_QUEUE_EMPTY;
    }

    memcpy(p_ptEvent,
        &p_ptQueue->t_tEvents[p_ptQueue->t_ulHead],
        sizeof(xos_event_t));

    p_ptQueue->t_ulHead = (p_ptQueue->t_ulHead + 1) % XOS_EVENT_QUEUE_MAX_SIZE;
    p_ptQueue->t_ulCount--;

    return XOS_EVENT_QUEUE_OK;
}

bool xEventQueueIsEmpty(xos_event_queue_t* p_ptQueue)
{
    X_ASSERT(p_ptQueue != NULL);
    X_ASSERT(p_ptQueue->t_bInitialized == true);

    return (p_ptQueue->t_ulCount == 0);
}

bool xEventQueueIsFull(xos_event_queue_t* p_ptQueue)
{
    X_ASSERT(p_ptQueue != NULL);
    X_ASSERT(p_ptQueue->t_bInitialized == true);

    return (p_ptQueue->t_ulCount >= XOS_EVENT_QUEUE_MAX_SIZE);
}

int xEventQueueGetStats(xos_event_queue_t* p_ptQueue,
    uint32_t* p_pulCount,
    uint32_t* p_pulDropped)
{
    X_ASSERT(p_ptQueue != NULL);
    X_ASSERT(p_pulCount != NULL);
    X_ASSERT(p_pulDropped != NULL);
    X_ASSERT(p_ptQueue->t_bInitialized == true);

    *p_pulCount = p_ptQueue->t_ulCount;
    *p_pulDropped = p_ptQueue->t_ulDropped;

    return XOS_EVENT_QUEUE_OK;
}
