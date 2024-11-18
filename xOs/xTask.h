////////////////////////////////////////////////////////////
//  os Task header file
//  defines the os function for thread manipulation
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
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xAssert.h"

#define OS_TASK_SUCCESS 0
#define OS_TASK_ERROR -1

#define OS_TASK_LOWEST_PRIORITY 0
#define OS_TASK_HIGHEST_PRIORITY 31

#define OS_TASK_DEFAULT_STACK_SIZE 0
#define OS_TASK_DEFAULT_PRIORITY 15
















#endif // OS_TASK_H_