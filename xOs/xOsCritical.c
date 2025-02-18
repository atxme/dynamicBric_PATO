#include "xOsCritical.h"
#include <errno.h>
#include <time.h>

////////////////////////////////////////////////////////////
/// osCriticalCreate
////////////////////////////////////////////////////////////
int osCriticalCreate(os_critical_t* p_pttOSCritical)
{
    X_ASSERT(p_pttOSCritical != NULL);

    pthread_mutexattr_t l_tAttr;
    pthread_mutexattr_init(&l_tAttr);
    pthread_mutexattr_settype(&l_tAttr, PTHREAD_MUTEX_RECURSIVE_NP);
        

    if (pthread_mutex_init(&p_pttOSCritical->critical, &l_tAttr) != 0) {
        pthread_mutexattr_destroy(&l_tAttr);
        return OS_CRITICAL_ERROR;
    }

    pthread_mutexattr_destroy(&l_tAttr);
    p_pttOSCritical->iLock = OS_CRITICAL_UNLOCKED;
    p_pttOSCritical->ulTimeout = OS_CRITICAL_DEFAULT_TIMEOUT;

    return OS_CRITICAL_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osCriticalLock
////////////////////////////////////////////////////////////
int osCriticalLock(os_critical_t* p_pttOSCritical) 
{
    X_ASSERT(p_pttOSCritical != NULL);

    if (pthread_mutex_lock(&p_pttOSCritical->critical) != 0) {
        return OS_CRITICAL_ERROR;
    }

    p_pttOSCritical->iLock = OS_CRITICAL_LOCKED;
    return OS_CRITICAL_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osCriticalLockTimeout
////////////////////////////////////////////////////////////
int osCriticalLockWithTimeout(os_critical_t* p_pttOSCritical)
{
    X_ASSERT(p_pttOSCritical != NULL);
	X_ASSERT(p_pttOSCritical->ulTimeout != OS_CRITICAL_DEFAULT_TIMEOUT);    // Timeout must be set

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return OS_CRITICAL_ERROR;
    }

    uint64_t nsec = (uint64_t)ts.tv_nsec + (uint64_t)(p_pttOSCritical->ulTimeout % 1000) * 1000000ULL;
    ts.tv_sec += p_pttOSCritical->ulTimeout / 1000 + (nsec / 1000000000ULL);
    ts.tv_nsec = nsec % 1000000000ULL;

    int result = pthread_mutex_timedlock(&p_pttOSCritical->critical, &ts);
    if (result == ETIMEDOUT) {
        return OS_CRITICAL_TIMEOUT;
    }
    else if (result != 0) {
        return OS_CRITICAL_ERROR;
    }

    p_pttOSCritical->iLock = OS_CRITICAL_LOCKED;
    return OS_CRITICAL_SUCCESS;
}


////////////////////////////////////////////////////////////
/// osCriticalUnlock
////////////////////////////////////////////////////////////
int osCriticalUnlock(os_critical_t* p_pttOSCritical)
{
    X_ASSERT(p_pttOSCritical != NULL);

    if (pthread_mutex_unlock(&p_pttOSCritical->critical) != 0) {
        return OS_CRITICAL_ERROR;
    }

    p_pttOSCritical->iLock = OS_CRITICAL_UNLOCKED;
    return OS_CRITICAL_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osCriticalDestroy
////////////////////////////////////////////////////////////
int osCriticalDestroy(os_critical_t* p_pttOSCritical)
{
    X_ASSERT(p_pttOSCritical != NULL);

    if (pthread_mutex_destroy(&p_pttOSCritical->critical) != 0) {
        return OS_CRITICAL_ERROR;
    }

    p_pttOSCritical->iLock = OS_CRITICAL_UNLOCKED;
    return OS_CRITICAL_SUCCESS;
}
