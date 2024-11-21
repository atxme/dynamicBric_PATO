////////////////////////////////////////////////////////////
//  os critical header file
//  defines the os function for critical section manipulation
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////
#pragma once

#ifndef OS_CRITICAL_H_
#define OS_CRITICAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "xAssert.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <linux/time.h>
#include <time.h>
#endif // _WIN32

#define OS_CRITICAL_SUCCESS 0
#define OS_CRITICAL_ERROR -1

#define OS_CRITICAL_LOCKED 0
#define OS_CRITICAL_UNLOCKED 1

#define OS_CRITICAL_DEFAULT_TIMEOUT 0

//////////////////////////////////
/// @brief critical struct creation
/// @param lock critical lock status
/// @param timeout critical timeout 
/// @note the structure is used for all the management of the critical section
/// @note the critical section is used to protect the shared resources
//////////////////////////////////
typedef struct {
	int iLock;   // lock status
	unsigned long ulTimeout; // timeout in ms 
#ifdef _WIN32
	CRITICAL_SECTION critical; // critical section
#else
	pthread_mutex_t critical; // critical section
#endif // _WIN32
} os_critical_t;

//////////////////////////////////
/// @brief osCriticalCreate
/// @param p_pttOSCritical pointer to the critical section
/// @return OS_CRITICAL_SUCCESS if success, OS_CRITICAL_ERROR otherwise
/// @note create the critical section
/// @note timeout is set to 0
//////////////////////////////////
int osCriticalCreate(os_critical_t* p_pttOSCritical);


//////////////////////////////////
/// @brief osCriticalLock
/// @param p_pttOSCritical pointer to the critical section
/// @return OS_CRITICAL_SUCCESS if success, OS_CRITICAL_ERROR otherwise
/// @note lock the critical section
/////////////////////////////////
int osCriticalLock(os_critical_t* p_pttOSCritical);

//////////////////////////////////
/// @brief osCriticalLockWithTimeout
/// @param p_pttOSCritical pointer to the critical section
/// @return OS_CRITICAL_SUCCESS if success, OS_CRITICAL_ERROR otherwise*
/// @note lock the critical section with timeout
///////////////////////////////////
int osCriticalLockWithTimeout(os_critical_t* p_pttOSCritical);

//////////////////////////////////
/// @brief osCriticalUnlock
/// @param p_pttOSCritical pointer to the critical section
/// @return OS_CRITICAL_SUCCESS if success, OS_CRITICAL_ERROR otherwise
/// @note unlock the critical section
//////////////////////////////////
int osCriticalUnlock(os_critical_t* p_pttOSCritical);

//////////////////////////////////
/// @brief osCriticalDestroy
/// @param p_pttOSCritical pointer to the critical section
/// @return OS_CRITICAL_SUCCESS if success, OS_CRITICAL_ERROR otherwise
/// @note destroy the critical section
//////////////////////////////////
int osCriticalDestroy(os_critical_t* p_pttOSCritical);


#endif // OS_CRITICAL_H_