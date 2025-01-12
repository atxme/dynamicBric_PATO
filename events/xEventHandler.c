////////////////////////////////////////////////////////////
//  event handler source file
//  implements event handler functions
//
// general discloser: copy or share the file is forbidden
// Written : 12/01/2025
////////////////////////////////////////////////////////////

#include "xEventHandler.h"
#include "xAssert.h"
#include <string.h>

// Static subscriber array
static xos_event_subscriber_t s_tSubscribers[XOS_EVENT_HANDLER_MAX_SUBSCRIBERS];
static bool s_bInitialized = false;

int xEventHandlerInit(void)
{
    X_ASSERT(s_bInitialized == false);

    memset(s_tSubscribers, 0, sizeof(s_tSubscribers));
    s_bInitialized = true;

    return XOS_EVENT_HANDLER_OK;
}

int xEventHandlerSubscribe(uint32_t p_ulEventId,
    xos_event_callback_t p_pfCallback,
    void* p_ptArg,
    uint8_t p_ucFlags)
{
    X_ASSERT(s_bInitialized == true);
    X_ASSERT(p_pfCallback != NULL);

    // Vérifier si déjà souscrit
    for (uint32_t i = 0; i < XOS_EVENT_HANDLER_MAX_SUBSCRIBERS; i++)
    {
        if (s_tSubscribers[i].t_bActive &&
            s_tSubscribers[i].t_ulEventId == p_ulEventId &&
            s_tSubscribers[i].t_pfCallback == p_pfCallback)
        {
            return XOS_EVENT_HANDLER_ALREADY_EXISTS;
        }
    }

    // Trouver un emplacement libre
    for (uint32_t i = 0; i < XOS_EVENT_HANDLER_MAX_SUBSCRIBERS; i++)
    {
        if (!s_tSubscribers[i].t_bActive)
        {
            s_tSubscribers[i].t_ulEventId = p_ulEventId;
            s_tSubscribers[i].t_pfCallback = p_pfCallback;
            s_tSubscribers[i].t_ptArg = p_ptArg;
            s_tSubscribers[i].t_ucFlags = p_ucFlags;
            s_tSubscribers[i].t_bActive = true;
            return XOS_EVENT_HANDLER_OK;
        }
    }

    return XOS_EVENT_HANDLER_FULL;
}

int xEventHandlerUnsubscribe(uint32_t p_ulEventId,
    xos_event_callback_t p_pfCallback)
{
    X_ASSERT(s_bInitialized == true);
    X_ASSERT(p_pfCallback != NULL);

    for (uint32_t i = 0; i < XOS_EVENT_HANDLER_MAX_SUBSCRIBERS; i++)
    {
        if (s_tSubscribers[i].t_bActive &&
            s_tSubscribers[i].t_ulEventId == p_ulEventId &&
            s_tSubscribers[i].t_pfCallback == p_pfCallback)
        {
            memset(&s_tSubscribers[i], 0, sizeof(xos_event_subscriber_t));
            return XOS_EVENT_HANDLER_OK;
        }
    }

    return XOS_EVENT_HANDLER_NOT_FOUND;
}

int xEventHandlerProcess(xos_event_t* p_ptEvent)
{
    X_ASSERT(s_bInitialized == true);
    X_ASSERT(p_ptEvent != NULL);

    for (uint32_t i = 0; i < XOS_EVENT_HANDLER_MAX_SUBSCRIBERS; i++)
    {
        if (s_tSubscribers[i].t_bActive &&
            s_tSubscribers[i].t_ulEventId == p_ptEvent->t_ulEventId)
        {
            s_tSubscribers[i].t_pfCallback(p_ptEvent, s_tSubscribers[i].t_ptArg);
        }
    }

    return XOS_EVENT_HANDLER_OK;
}

int xEventHandlerCleanup(void)
{
    X_ASSERT(s_bInitialized == true);

    memset(s_tSubscribers, 0, sizeof(s_tSubscribers));
    s_bInitialized = false;

    return XOS_EVENT_HANDLER_OK;
}
