////////////////////////////////////////////////////////////
//  timer source file
//  implements the timer functions
//
// general discloser: copy or share the file is forbidden
// Written : 12/01/2025
////////////////////////////////////////////////////////////

#include "xTimer.h"
#include "xAssert.h"
#include <string.h>

int xTimerCreate(xos_timer_t* p_ptTimer, uint32_t p_ulPeriod, uint8_t p_ucMode)
{
    X_ASSERT(p_ptTimer != NULL);
    X_ASSERT(p_ulPeriod > 0);
    X_ASSERT(p_ucMode <= XOS_TIMER_MODE_PERIODIC);
    
    memset(p_ptTimer, 0, sizeof(xos_timer_t));
    p_ptTimer->t_ulPeriod = p_ulPeriod;
    p_ptTimer->t_ucMode = p_ucMode;
    p_ptTimer->t_ucActive = 0;
    
    return XOS_TIMER_OK;
}

int xTimerStart(xos_timer_t* p_ptTimer)
{
    X_ASSERT(p_ptTimer != NULL);
    
    clock_gettime(CLOCK_MONOTONIC, &p_ptTimer->t_tStart);
    p_ptTimer->t_tNext = p_ptTimer->t_tStart;
    p_ptTimer->t_tNext.tv_nsec += (p_ptTimer->t_ulPeriod * 1000);
    
    if (p_ptTimer->t_tNext.tv_nsec >= 1000000000)
    {
        p_ptTimer->t_tNext.tv_sec += p_ptTimer->t_tNext.tv_nsec / 1000000000;
        p_ptTimer->t_tNext.tv_nsec %= 1000000000;
    }
    
    p_ptTimer->t_ucActive = 1;
    return XOS_TIMER_OK;
}

int xTimerStop(xos_timer_t* p_ptTimer)
{
    X_ASSERT(p_ptTimer != NULL);
    
    p_ptTimer->t_ucActive = 0;
    return XOS_TIMER_OK;
}

int xTimerExpired(xos_timer_t* p_ptTimer)
{
    X_ASSERT(p_ptTimer != NULL);
    
    if (!p_ptTimer->t_ucActive)
    {
        return XOS_TIMER_NOT_INIT;
    }
    
    struct timespec l_tNow;
    clock_gettime(CLOCK_MONOTONIC, &l_tNow);
    
    if ((l_tNow.tv_sec > p_ptTimer->t_tNext.tv_sec) ||
        (l_tNow.tv_sec == p_ptTimer->t_tNext.tv_sec && 
         l_tNow.tv_nsec >= p_ptTimer->t_tNext.tv_nsec))
    {
        if (p_ptTimer->t_ucMode == XOS_TIMER_MODE_PERIODIC)
        {
            p_ptTimer->t_tNext.tv_nsec += (p_ptTimer->t_ulPeriod * 1000);
            if (p_ptTimer->t_tNext.tv_nsec >= 1000000000)
            {
                p_ptTimer->t_tNext.tv_sec += p_ptTimer->t_tNext.tv_nsec / 1000000000;
                p_ptTimer->t_tNext.tv_nsec %= 1000000000;
            }
        }
        else
        {
            p_ptTimer->t_ucActive = 0;
        }
        
        return XOS_TIMER_OK;
    }
    
    return XOS_TIMER_TIMEOUT;
}
