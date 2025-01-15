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
#include "xOsMutex.h"
#include "xTimer.h"

// Static subscriber array
static xos_event_subscriber_t s_tSubscribers[XOS_EVENT_HANDLER_MAX_SUBSCRIBERS];
static bool s_bInitialized = false;
static xos_mutex_t s_tHandlerMutex;
static const uint32_t EVENT_CALLBACK_TIMEOUT_MS = 1000; // 1 seconde max par callback

int xEventHandlerInit(void)
{
    X_ASSERT(s_bInitialized == false);

    int l_iRet = mutexCreate(&s_tHandlerMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_EVENT_HANDLER_ERROR;
    }

    l_iRet = mutexLock(&s_tHandlerMutex);
    if (l_iRet != MUTEX_OK)
    {
        mutexDestroy(&s_tHandlerMutex);
        return XOS_EVENT_HANDLER_ERROR;
    }

    memset(s_tSubscribers, 0, sizeof(s_tSubscribers));
    s_bInitialized = true;

    mutexUnlock(&s_tHandlerMutex);
    return XOS_EVENT_HANDLER_OK;
}

int xEventHandlerSubscribe(uint32_t p_ulEventId,
    xos_event_callback_t p_pfCallback,
    void* p_ptArg,
    uint8_t p_ucFlags)
{
    X_ASSERT(p_pfCallback != NULL);

    int l_iRet = mutexLock(&s_tHandlerMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_EVENT_HANDLER_ERROR;
    }

    if (!s_bInitialized)
    {
        mutexUnlock(&s_tHandlerMutex);
        return XOS_EVENT_HANDLER_NOT_INIT;
    }

    // Vérifier si déjà souscrit
    for (uint32_t i = 0; i < XOS_EVENT_HANDLER_MAX_SUBSCRIBERS; i++)
    {
        if (s_tSubscribers[i].t_bActive &&
            s_tSubscribers[i].t_ulEventId == p_ulEventId &&
            s_tSubscribers[i].t_pfCallback == p_pfCallback)
        {
            mutexUnlock(&s_tHandlerMutex);
            return XOS_EVENT_HANDLER_ALREADY_EXISTS;
        }
    }

    // Trouver un emplacement libre
    int l_iIndex = -1;
    for (uint32_t i = 0; i < XOS_EVENT_HANDLER_MAX_SUBSCRIBERS; i++)
    {
        if (!s_tSubscribers[i].t_bActive)
        {
            l_iIndex = i;
            break;
        }
    }

    if (l_iIndex < 0)
    {
        mutexUnlock(&s_tHandlerMutex);
        return XOS_EVENT_HANDLER_FULL;
    }

    // Copie sécurisée des données
    s_tSubscribers[l_iIndex].t_ulEventId = p_ulEventId;
    s_tSubscribers[l_iIndex].t_pfCallback = p_pfCallback;
    s_tSubscribers[l_iIndex].t_ptArg = p_ptArg;
    s_tSubscribers[l_iIndex].t_ucFlags = p_ucFlags;
    s_tSubscribers[l_iIndex].t_bActive = true;

    mutexUnlock(&s_tHandlerMutex);
    return XOS_EVENT_HANDLER_OK;
}

int xEventHandlerUnsubscribe(uint32_t p_ulEventId,
    xos_event_callback_t p_pfCallback)
{
    X_ASSERT(p_pfCallback != NULL);

    int l_iRet = mutexLock(&s_tHandlerMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_EVENT_HANDLER_ERROR;
    }

    if (!s_bInitialized)
    {
        mutexUnlock(&s_tHandlerMutex);
        return XOS_EVENT_HANDLER_NOT_INIT;
    }

    bool l_bFound = false;
    for (uint32_t i = 0; i < XOS_EVENT_HANDLER_MAX_SUBSCRIBERS; i++)
    {
        if (s_tSubscribers[i].t_bActive &&
            s_tSubscribers[i].t_ulEventId == p_ulEventId &&
            s_tSubscribers[i].t_pfCallback == p_pfCallback)
        {
            memset(&s_tSubscribers[i], 0, sizeof(xos_event_subscriber_t));
            l_bFound = true;
            break;
        }
    }

    mutexUnlock(&s_tHandlerMutex);
    return l_bFound ? XOS_EVENT_HANDLER_OK : XOS_EVENT_HANDLER_NOT_FOUND;
}

static int validateEvent(const xos_event_t* p_ptEvent)
{
    if (p_ptEvent->t_ulDataSize > 0 && p_ptEvent->t_ptData == NULL)
    {
        return XOS_EVENT_HANDLER_INVALID;
    }

    // Vérifier les limites des énumérations
    if (p_ptEvent->t_eType > XOS_EVENT_TYPE_IO ||
        p_ptEvent->t_tPrio > XOS_EVENT_PRIORITY_SYSTEM)
    {
        return XOS_EVENT_HANDLER_INVALID;
    }

    return XOS_EVENT_HANDLER_OK;
}

int xEventHandlerProcess(xos_event_t* p_ptEvent)
{
    X_ASSERT(p_ptEvent != NULL);
    
    // Copie locale des subscribers
    xos_event_subscriber_t l_tLocalSubscribers[XOS_EVENT_HANDLER_MAX_SUBSCRIBERS];
    uint32_t l_ulActiveCount = 0;
    
    int l_iRet = mutexLock(&s_tHandlerMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_EVENT_HANDLER_ERROR;
    }

    if (!s_bInitialized)
    {
        mutexUnlock(&s_tHandlerMutex);
        return XOS_EVENT_HANDLER_NOT_INIT;
    }

    // Valider l'événement
    l_iRet = validateEvent(p_ptEvent);
    if (l_iRet != XOS_EVENT_HANDLER_OK)
    {
        mutexUnlock(&s_tHandlerMutex);
        return l_iRet;
    }

    // Copier les subscribers actifs
    for (uint32_t i = 0; i < XOS_EVENT_HANDLER_MAX_SUBSCRIBERS; i++)
    {
        if (s_tSubscribers[i].t_bActive &&
            s_tSubscribers[i].t_ulEventId == p_ptEvent->t_ulEventId)
        {
            // Incrémenter le compteur de référence
            s_tSubscribers[i].t_ulRefCount++;
            memcpy(&l_tLocalSubscribers[l_ulActiveCount], 
                   &s_tSubscribers[i], 
                   sizeof(xos_event_subscriber_t));
            l_ulActiveCount++;
        }
    }

    mutexUnlock(&s_tHandlerMutex);

    // Exécuter les callbacks
    for (uint32_t i = 0; i < l_ulActiveCount; i++)
    {
        if (l_tLocalSubscribers[i].t_pfCallback != NULL)
        {
            l_tLocalSubscribers[i].t_pfCallback(p_ptEvent, l_tLocalSubscribers[i].t_ptArg);
        }
        
        // Décrémenter le compteur de référence
        mutexLock(&s_tHandlerMutex);
        for (uint32_t j = 0; j < XOS_EVENT_HANDLER_MAX_SUBSCRIBERS; j++)
        {
            if (s_tSubscribers[j].t_ulEventId == l_tLocalSubscribers[i].t_ulEventId &&
                s_tSubscribers[j].t_pfCallback == l_tLocalSubscribers[i].t_pfCallback)
            {
                s_tSubscribers[j].t_ulRefCount--;
                break;
            }
        }
        mutexUnlock(&s_tHandlerMutex);
    }

    return XOS_EVENT_HANDLER_OK;
}

int xEventHandlerCleanup(void)
{
    int l_iRet = mutexLock(&s_tHandlerMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_EVENT_HANDLER_ERROR;
    }

    if (!s_bInitialized)
    {
        mutexUnlock(&s_tHandlerMutex);
        return XOS_EVENT_HANDLER_NOT_INIT;
    }

    memset(s_tSubscribers, 0, sizeof(s_tSubscribers));
    s_bInitialized = false;

    mutexUnlock(&s_tHandlerMutex);
    l_iRet = mutexDestroy(&s_tHandlerMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_EVENT_HANDLER_ERROR;
    }

    return XOS_EVENT_HANDLER_OK;
}
