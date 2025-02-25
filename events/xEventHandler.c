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
#include "timer/xTimer.h"

//////////////////////////////////
// Static variables
//////////////////////////////////
static xos_event_subscriber_t s_tSubscribers[XOS_EVENT_HANDLER_MAX_SUBSCRIBERS];
static bool s_bInitialized = false;
static t_MutexCtx s_tHandlerMutex;
static const uint32_t EVENT_CALLBACK_TIMEOUT_MS = 1000;

//////////////////////////////////
///@brief Static function declarations
///@param p_ptEvent : structure event pointer
//////////////////////////////////
static int validateEvent(const xos_event_t* p_ptEvent);

//////////////////////////////////
// xEventHandlerInit
//////////////////////////////////
int xEventHandlerInit(void)
{
    int l_iRet;

    X_ASSERT(s_bInitialized == false);

    // Cr�ation du mutex
    l_iRet = mutexCreate(&s_tHandlerMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_EVENT_HANDLER_ERROR;
    }

    // Verrouillage du mutex
    l_iRet = mutexLock(&s_tHandlerMutex);
    if (l_iRet != MUTEX_OK)
    {
        mutexDestroy(&s_tHandlerMutex);
        return XOS_EVENT_HANDLER_ERROR;
    }

    // Initialisation du tableau des abonn�s
    memset(s_tSubscribers, 0, sizeof(s_tSubscribers));
    s_bInitialized = true;

    // D�verrouillage du mutex
    mutexUnlock(&s_tHandlerMutex);
    return XOS_EVENT_HANDLER_OK;
}

//////////////////////////////////
// xEventHandlerSubscribe
//////////////////////////////////
int xEventHandlerSubscribe(uint32_t p_ulEventId,
                           xos_event_callback_t p_pfCallback,
                           void* p_ptArg,
                           uint8_t p_ucFlags)
{
    int l_iRet;
    int l_iIndex = -1;

	// Check parameters
    X_ASSERT(p_pfCallback != NULL);

    // Lock mutex
    l_iRet = mutexLock(&s_tHandlerMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_EVENT_HANDLER_ERROR;
    }

	// Check initialization
    if (!s_bInitialized)
    {
        mutexUnlock(&s_tHandlerMutex);
        return XOS_EVENT_HANDLER_NOT_INIT;
    }

	// Check if subscriber already exists
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

	// Find free subscriber slot
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

	// Init subscriber
    s_tSubscribers[l_iIndex].t_ulEventId = p_ulEventId;
    s_tSubscribers[l_iIndex].t_pfCallback = p_pfCallback;
    s_tSubscribers[l_iIndex].t_ptArg = p_ptArg;
    s_tSubscribers[l_iIndex].t_ucFlags = p_ucFlags;
    s_tSubscribers[l_iIndex].t_bActive = true;
    s_tSubscribers[l_iIndex].t_ulRefCount = 0;

    mutexUnlock(&s_tHandlerMutex);
    return XOS_EVENT_HANDLER_OK;

}

//////////////////////////////////
// xEventHandlerUnsubscribe
//////////////////////////////////
int xEventHandlerUnsubscribe(uint32_t p_ulEventId,
    xos_event_callback_t p_pfCallback)
{
    int l_iRet;
    bool l_bFound = false;

    // Validation du param�tre callback
    X_ASSERT(p_pfCallback != NULL);

    // Verrouillage du mutex
    l_iRet = mutexLock(&s_tHandlerMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_EVENT_HANDLER_ERROR;
    }

    // V�rification de l'initialisation
    if (!s_bInitialized)
    {
        mutexUnlock(&s_tHandlerMutex);
        return XOS_EVENT_HANDLER_NOT_INIT;
    }

    // Recherche et suppression de l'abonn�
    for (uint32_t i = 0; i < XOS_EVENT_HANDLER_MAX_SUBSCRIBERS; i++)
    {
        if (s_tSubscribers[i].t_bActive &&
            s_tSubscribers[i].t_ulEventId == p_ulEventId &&
            s_tSubscribers[i].t_pfCallback == p_pfCallback)
        {
            // Attente que les callbacks en cours se terminent
            while (s_tSubscribers[i].t_ulRefCount > 0)
            {
                mutexUnlock(&s_tHandlerMutex);
                xTimerDelay(1);
                mutexLock(&s_tHandlerMutex);
            }

            memset(&s_tSubscribers[i], 0, sizeof(xos_event_subscriber_t));
            l_bFound = true;
            break;
        }
    }

    mutexUnlock(&s_tHandlerMutex);
    return l_bFound ? XOS_EVENT_HANDLER_OK : XOS_EVENT_HANDLER_NOT_FOUND;
}

//////////////////////////////////
// xEventHandlerProcess
//////////////////////////////////
int xEventHandlerProcess(xos_event_t* p_ptEvent)
{
    int l_iRet;
    xos_event_subscriber_t l_tLocalSubscribers[XOS_EVENT_HANDLER_MAX_SUBSCRIBERS];
    uint32_t l_ulActiveCount = 0;
    uint32_t l_ulStartTime;
    bool l_bTimeoutOccured = false;

    // Validation du param�tre event
    X_ASSERT(p_ptEvent != NULL);

    // Verrouillage du mutex
    l_iRet = mutexLock(&s_tHandlerMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_EVENT_HANDLER_ERROR;
    }

    // V�rification de l'initialisation
    if (!s_bInitialized)
    {
        mutexUnlock(&s_tHandlerMutex);
        return XOS_EVENT_HANDLER_NOT_INIT;
    }

    // Validation de l'�v�nement
    l_iRet = validateEvent(p_ptEvent);
    if (l_iRet != XOS_EVENT_HANDLER_OK)
    {
        mutexUnlock(&s_tHandlerMutex);
        return l_iRet;
    }

    // Copie des abonn�s actifs correspondant � l'�v�nement et incr�ment du compteur de r�f�rence
    for (uint32_t i = 0; i < XOS_EVENT_HANDLER_MAX_SUBSCRIBERS; i++)
    {
        if (s_tSubscribers[i].t_bActive &&
            s_tSubscribers[i].t_ulEventId == p_ptEvent->t_ulEventId)
        {
            s_tSubscribers[i].t_ulRefCount++;
            memcpy(&l_tLocalSubscribers[l_ulActiveCount],
                &s_tSubscribers[i],
                sizeof(xos_event_subscriber_t));
            l_ulActiveCount++;
        }
    }

    mutexUnlock(&s_tHandlerMutex);

    // Ex�cution des callbacks
    for (uint32_t i = 0; i < l_ulActiveCount; i++)
    {
        if (l_tLocalSubscribers[i].t_pfCallback != NULL)
        {
            l_ulStartTime = xTimerGetCurrentMs();

            // Appel de la fonction callback
            l_tLocalSubscribers[i].t_pfCallback(p_ptEvent, l_tLocalSubscribers[i].t_ptArg);

            // Calcul du temps d'ex�cution
            uint32_t execTime = xTimerGetCurrentMs() - l_ulStartTime;

            // Toujours d�cr�menter le compteur de r�f�rence,
            // m�me en cas de d�passement (timeout)
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

            // Si la dur�e du callback d�passe le d�lai imparti, on note l'erreur
            if (execTime > EVENT_CALLBACK_TIMEOUT_MS)
            {
                l_bTimeoutOccured = true;
            }
        }
    }

    return l_bTimeoutOccured ? XOS_EVENT_HANDLER_TIMEOUT : XOS_EVENT_HANDLER_OK;
}

//////////////////////////////////
// xEventHandlerCleanup
//////////////////////////////////
int xEventHandlerCleanup(void)
{
    int l_iRet;

    // Verrouillage du mutex
    l_iRet = mutexLock(&s_tHandlerMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_EVENT_HANDLER_ERROR;
    }

    // V�rification de l'initialisation
    if (!s_bInitialized)
    {
        mutexUnlock(&s_tHandlerMutex);
        return XOS_EVENT_HANDLER_NOT_INIT;
    }

    // Attente que tous les callbacks en cours soient termin�s
    for (uint32_t i = 0; i < XOS_EVENT_HANDLER_MAX_SUBSCRIBERS; i++)
    {
        while (s_tSubscribers[i].t_ulRefCount > 0)
        {
            mutexUnlock(&s_tHandlerMutex);
			xTimerDelay(1); /// TODO : remplacer par une attente active
            mutexLock(&s_tHandlerMutex);
        }
    }

    // R�initialisation des abonn�s
    memset(s_tSubscribers, 0, sizeof(s_tSubscribers));
    s_bInitialized = false;

    // D�verrouillage du mutex avant destruction
    mutexUnlock(&s_tHandlerMutex);

    // Destruction du mutex
    l_iRet = mutexDestroy(&s_tHandlerMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_EVENT_HANDLER_ERROR;
    }

    return XOS_EVENT_HANDLER_OK;
}

//////////////////////////////////
// validateEvent
//////////////////////////////////
static int validateEvent(const xos_event_t* p_ptEvent)
{
    // V�rification de la taille des donn�es et du pointeur associ�
    if (p_ptEvent->t_ulDataSize > 0 && p_ptEvent->t_ptData == NULL)
    {
        return XOS_EVENT_HANDLER_INVALID;
    }

    // V�rification du type d'�v�nement et de sa priorit�
    if (p_ptEvent->t_eType > XOS_EVENT_TYPE_IO ||
        p_ptEvent->t_tPrio > XOS_EVENT_PRIORITY_SYSTEM)
    {
        return XOS_EVENT_HANDLER_INVALID;
    }

    return XOS_EVENT_HANDLER_OK;
}

