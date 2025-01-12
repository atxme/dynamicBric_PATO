////////////////////////////////////////////////////////////
//  event source file
//  implements event functions
//
// general discloser: copy or share the file is forbidden
// Written : 12/01/2025
////////////////////////////////////////////////////////////

#include "xEvent.h"
#include "xAssert.h"
#include "xOsHorodateur.h"
#include <string.h>

// Static event queue
static xos_event_t s_tEvents[XOS_EVENT_MAX_EVENTS];
static uint32_t s_ulEventCount = 0;
static bool s_bInitialized = false;

int xEventInit(void)
{
    X_ASSERT(s_bInitialized == false);

    memset(s_tEvents, 0, sizeof(s_tEvents));
    s_ulEventCount = 0;
    s_bInitialized = true;

    return XOS_EVENT_OK;
}

int xEventPublish(uint32_t p_ulEventId, xos_event_type_t p_eType,
    xos_event_priority_t p_tPrio, uint8_t p_ucFlags,
    void* p_ptData, size_t p_ulSize)
{
    X_ASSERT(s_bInitialized == true);
    X_ASSERT(p_ulSize == 0 || p_ptData != NULL);

    if (s_ulEventCount >= XOS_EVENT_MAX_EVENTS)
    {
        return XOS_EVENT_FULL;
    }

    // Trouver un emplacement libre
    uint32_t l_ulIndex;
    for (l_ulIndex = 0; l_ulIndex < XOS_EVENT_MAX_EVENTS; l_ulIndex++)
    {
        if (s_tEvents[l_ulIndex].t_ulEventId == 0)
        {
            break;
        }
    }

    if (l_ulIndex >= XOS_EVENT_MAX_EVENTS)
    {
        return XOS_EVENT_FULL;
    }

    // Initialiser l'événement
    s_tEvents[l_ulIndex].t_ulEventId = p_ulEventId;
    s_tEvents[l_ulIndex].t_eType = p_eType;
    s_tEvents[l_ulIndex].t_tPrio = p_tPrio;
    s_tEvents[l_ulIndex].t_ucFlags = p_ucFlags;
    s_tEvents[l_ulIndex].t_ptData = p_ptData;
    s_tEvents[l_ulIndex].t_ulDataSize = p_ulSize;
    s_tEvents[l_ulIndex].t_ulSequence = s_ulEventCount;
    s_tEvents[l_ulIndex].t_ulTimestamp = (uint32_t)xHorodateurGet();

    s_ulEventCount++;

    return XOS_EVENT_OK;
}

int xEventProcess(void)
{
    X_ASSERT(s_bInitialized == true);

    if (s_ulEventCount == 0)
    {
        return XOS_EVENT_OK;
    }

    // Traiter les événements par priorité
    for (xos_event_priority_t l_tPrio = XOS_EVENT_PRIORITY_SYSTEM;
        l_tPrio >= XOS_EVENT_PRIORITY_LOW;
        l_tPrio--)
    {
        for (uint32_t i = 0; i < XOS_EVENT_MAX_EVENTS; i++)
        {
            if (s_tEvents[i].t_ulEventId != 0 &&
                s_tEvents[i].t_tPrio == l_tPrio)
            {
                // Traiter l'événement
                if (!(s_tEvents[i].t_ucFlags & XOS_EVENT_FLAG_PERSISTENT))
                {
                    memset(&s_tEvents[i], 0, sizeof(xos_event_t));
                    s_ulEventCount--;
                }
            }
        }
    }

    return XOS_EVENT_OK;
}
