////////////////////////////////////////////////////////////
//  mutex header file
//  defines the mutex types and constants
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////
#pragma once

#ifndef XOS_MUTEX_H_
#define XOS_MUTEX_H_

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <errno.h>
#include <time.h>
#endif

// Mutex error codes
#define MUTEX_OK              0
#define MUTEX_ERROR         -1
#define MUTEX_TIMEOUT      -2
#define MUTEX_INVALID      -3
#define MUTEX_ALREADY_INIT -4
#define MUTEX_NOT_INIT     -5

// Mutex states
#define MUTEX_UNLOCKED 0
#define MUTEX_LOCKED   1

// Default timeout value (ms)
#define MUTEX_DEFAULT_TIMEOUT 1000

//////////////////////////////////
/// @brief mutex structure
/// @param t_mutex : mutex handle
/// @param t_iState : mutex state
/// @param t_ulTimeout : timeout value in milliseconds
//////////////////////////////////
typedef struct {
#ifdef _WIN32
    HANDLE t_mutex;
#else
    pthread_mutex_t t_mutex;
#endif
    int t_iState;
    unsigned long t_ulTimeout;
} xos_mutex_t;

//////////////////////////////////
/// @brief create mutex
/// @param p_ptMutex : mutex structure pointer
/// @return : success or error code
//////////////////////////////////
int mutexCreate(xos_mutex_t* p_ptMutex);

//////////////////////////////////
/// @brief lock mutex
/// @param p_ptMutex : mutex structure pointer
/// @return : success or error code
//////////////////////////////////
int mutexLock(xos_mutex_t* p_ptMutex);

//////////////////////////////////
/// @brief try to lock mutex
/// @param p_ptMutex : mutex structure pointer
/// @return : success or error code
//////////////////////////////////
int mutexTryLock(xos_mutex_t* p_ptMutex);

//////////////////////////////////
/// @brief lock mutex with timeout
/// @param p_ptMutex : mutex structure pointer
/// @param p_ulTimeout : timeout value in milliseconds
/// @return : success or error code
//////////////////////////////////
int mutexLockTimeout(xos_mutex_t* p_ptMutex, unsigned long p_ulTimeout);

//////////////////////////////////
/// @brief unlock mutex
/// @param p_ptMutex : mutex structure pointer
/// @return : success or error code
//////////////////////////////////
int mutexUnlock(xos_mutex_t* p_ptMutex);

//////////////////////////////////
/// @brief destroy mutex
/// @param p_ptMutex : mutex structure pointer
/// @return : success or error code
//////////////////////////////////
int mutexDestroy(xos_mutex_t* p_ptMutex);

//////////////////////////////////
/// @brief set mutex timeout
/// @param p_ptMutex : mutex structure pointer
/// @param p_ulTimeout : timeout value in milliseconds
/// @return : success or error code
//////////////////////////////////
int mutexSetTimeout(xos_mutex_t* p_ptMutex, unsigned long p_ulTimeout);

//////////////////////////////////
/// @brief get mutex state
/// @param p_ptMutex : mutex structure pointer
/// @return : mutex state or error code
//////////////////////////////////
int mutexGetState(xos_mutex_t* p_ptMutex);

#endif // XOS_MUTEX_H_





