////////////////////////////////////////////////////////////
//  os Task header file
//  defines the os function for task manipulation
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////
#pragma once

#ifndef OS_TASK_H_
#define OS_TASK_H_


#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <stdbool.h>
#include <signal.h> // to manage manually task management not permitted by posix standard
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xAssert.h"

#define OS_TASK_SUCCESS 0
#define OS_TASK_ERROR -1

#define OS_TASK_STATUS_READY 0UL
#define OS_TASK_STATUS_RUNNING 1UL
#define OS_TASK_STATUS_BLOCKED 2UL
#define OS_TASK_STATUS_SUSPENDED 3UL
#define OS_TASK_STATUS_TERMINATED 4UL

#define OS_TASK_EXIT_SUCCESS 0UL
#define OS_TASK_EXIT_FAILURE -1UL

#ifndef _WIN32
#include <sched.h>
#define OS_TASK_LOWEST_PRIORITY sched_get_priority_min(SCHED_RR)
#define OS_TASK_HIGHEST_PRIORITY sched_get_priority_max(SCHED_RR)
#define OS_TASK_DEFAULT_PRIORITY ((OS_TASK_HIGHEST_PRIORITY + OS_TASK_LOWEST_PRIORITY) / 2)
#else
#define OS_TASK_LOWEST_PRIORITY 1
#define OS_TASK_HIGHEST_PRIORITY 99
#define OS_TASK_DEFAULT_PRIORITY 15
#endif

#define OS_TASK_DEFAULT_STACK_SIZE (8 * 1024 * 1024)  // 8 MB


//////////////////////////////////
/// @brief task struct creation 
/// @param task function pointer to the task
/// @param arg pointer to the task argument
/// @param priority task priority
/// @param stack_size task stack size
/// @param id task id forbidden to edit 
/// @param status task status forbidden to edit 
/// @param exit_code task exit code
/// @param handle task handle
/// @note the structure is used for all the management of the task 
//////////////////////////////////
typedef struct {
    void* (*task)(void*);   // Pointeur vers la fonction de la t�che
    void* arg;              // Arguments de la t�che
    int priority;           // Priorit� de la t�che (OS-specific)
    size_t stack_size;         // Taille de la pile (OS-specific)
    int id;                 // ID de la t�che
    int status;             // Statut de la t�che
    int exit_code;          // Code de sortie de la t�che
#ifdef _WIN32
    HANDLE handle;          // Handle du thread Windows
#else
    pthread_t handle;       // Handle du thread pthread
#endif
} os_task_t;

//////////////////////////////////
/// @brief task creation
/// @param task pointer to the task struct
/// @return OS_TASK_SUCCESS if the task is created successfully
//////////////////////////////////
int osTaskCreate(os_task_t* p_pttOSTask);

#ifdef _WIN32
//////////////////////////////////
/// @brief task start
/// @note if not windows os, the task is already running so the function will resume the task if it's suspended
/// @param task pointer to the task struct
/// @return OS_TASK_SUCCESS if the task is started successfully
//////////////////////////////////
int osTaskStart(os_task_t* p_pttOSTask);

//////////////////////////////////
/// @brief task suspend a specific task
/// @param task pointer to the task struct
/// @return OS_TASK_SUCCESS if the task is suspended successfully
//////////////////////////////////
int osTaskSuspend(os_task_t* p_pttOSTask);

//////////////////////////////////
/// @brief task resume a specific task
/// @param task pointer to the task struct
/// @return OS_TASK_SUCCESS if the task is resumed successfully or error code
////////////////////////////////// 
int osTaskResume(os_task_t* p_pttOSTask);
#endif

//////////////////////////////////
/// @brief end a specific task
/// @note destroy the task and free the memory allocated
/// @param task pointer to the task struct
/// @return OS_TASK_SUCCESS if the task is deleted successfully or error code
//////////////////////////////////
int osTaskEnd(os_task_t* p_pttOSTask);

//////////////////////////////////
/// @brief update task informations 
/// @note if you don't want to update a param just put NULL
/// @param task pointer to the task struct
/// @param p_ptiPriority pointer to the priority
/// @return OS_TASK_SUCCESS if the task is updated successfully or error code
//////////////////////////////////
int osTaskUpdatePriority(os_task_t* p_pttOSTask, int* p_ptiPriority);


//////////////////////////////////
/// @brief get the task exit code
/// @param task pointer to the task struct
/// @return the task exit code
/// @note the exit code is defined by the OS_TASK_EXIT_XXX
///////////////////////////////////
int osTaskGetExitCode(os_task_t* p_pttOSTask);



#endif // OS_TASK_H_