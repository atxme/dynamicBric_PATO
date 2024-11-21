////////////////////////////////////////////////////////////
//  os critical src file
//  defines the os function for critical section manipulation
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include "xOsCritical.h"

////////////////////////////////////////////////////////////
/// osCriticalCreate
////////////////////////////////////////////////////////////
int osCriticalCreate(os_critical_t* p_pttOSCritical)
{
	//check the critical pointer
	X_ASSERT(p_pttOSCritical != NULL);

	//convert to loccal structure
	os_critical_t tOSCritical = *p_pttOSCritical;

	//create the critical section
#ifdef _WIN32
	InitializeCriticalSection(&tOSCritical.critical);
#else
	pthread_mutex_init(&tOSCritical.critical, NULL);
#endif
	//set the critical lock status
	tOSCritical.iLock = OS_CRITICAL_UNLOCKED;

	//set the critical timeout
	tOSCritical.ulTimeout = OS_CRITICAL_DEFAULT_TIMEOUT;

	//set the critical pointer
	*p_pttOSCritical = tOSCritical;

	return OS_CRITICAL_SUCCESS;

}

////////////////////////////////////////////////////////////
/// osCriticalLock
////////////////////////////////////////////////////////////
int osCriticalLock(os_critical_t* p_pttOSCritical)
{
	//check the critical pointer
	X_ASSERT(p_pttOSCritical != NULL);
	X_ASSERT(p_pttOSCritical->iLock == OS_CRITICAL_UNLOCKED);

	//convert to loccal structure
	os_critical_t tOSCritical = *p_pttOSCritical;

	//lock the critical section
#ifdef _WIN32
	EnterCriticalSection(&tOSCritical.critical);
#else
	if (tOSCritical.ulTimeout == 0)
	{
		pthread_mutex_lock(&tOSCritical.critical);
	}
	else
	{
		unsigned long l_ulReturn = osCriticalLockWithTimeout(p_pttOSCritical);

		if ((int)l_ulReturn == OS_CRITICAL_ERROR)
		{
			return OS_CRITICAL_ERROR;
		}
	}
#endif
	//set the critical lock status
	tOSCritical.iLock = OS_CRITICAL_LOCKED;

	//set the critical pointer
	*p_pttOSCritical = tOSCritical;

	return OS_CRITICAL_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osCriticalUnlock
////////////////////////////////////////////////////////////
int osCriticalUnlock(os_critical_t* p_pttOSCritical)
{
	//check the critical pointer
	X_ASSERT(p_pttOSCritical != NULL);
	X_ASSERT(p_pttOSCritical->iLock == OS_CRITICAL_LOCKED);

	//convert to loccal structure
	os_critical_t tOSCritical = *p_pttOSCritical;

	//unlock the critical section
#ifdef _WIN32
	LeaveCriticalSection(&tOSCritical.critical);
#else
	pthread_mutex_unlock(&tOSCritical.critical);
#endif
	//set the critical lock status
	tOSCritical.iLock = OS_CRITICAL_UNLOCKED;

	//set the critical pointer
	*p_pttOSCritical = tOSCritical;

	return OS_CRITICAL_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osCriticalDestroy
////////////////////////////////////////////////////////////
int osCriticalDestroy(os_critical_t* p_pttOSCritical)
{
	//check the critical pointer
	X_ASSERT(p_pttOSCritical != NULL);

	//convert to loccal structure
	os_critical_t tOSCritical = *p_pttOSCritical;

	//destroy the critical section
#ifdef _WIN32
	DeleteCriticalSection(&tOSCritical.critical);
#else
	pthread_mutex_destroy(&tOSCritical.critical);
#endif
	//set the critical lock status
	tOSCritical.iLock = OS_CRITICAL_UNLOCKED;

	//set the critical pointer
	*p_pttOSCritical = tOSCritical;

	return OS_CRITICAL_SUCCESS;
}
