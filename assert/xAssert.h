////////////////////////////////////////////////////////////
//  assert header file
//  defines assert options
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else 
#include <unistd.h>
#endif

#define ASSERT_LOOP 0
#define ASSERT_EXIT 0
#define ASSERT_CONTINUE 0

#ifndef ASSERT_ASSERT_H_
#define ASSERT_ASSERT_H_

#include <stdint.h>

//////////////////////////////////
/// @brief assert function
/// @note this is the classic assert function
/// @param p_pFileName : file name
/// @param p_uiLine : line number
/// @param p_ptLog : log pointer
///////////////////////////////////
#define X_ASSERT(expr) ((expr) ? (void)0 : assert((uint8_t *)__FILE__, __LINE__, NULL));

//////////////////////////////////
/// @brief assert function
/// @note this is the assert function with log inserted on a file. The path of the file should be set before using this function
/// @param p_pFileName : file name
/// @param p_uiLine : line number
/// @param p_ptLog : log pointer
///////////////////////////////////
#define X_ASSERT_LOG (expr, p_ptLog) ((expr) ? (void)0 : assert((uint8_t *)__FILE__, __LINE__,  p_ptLog));

//////////////////////////////////
/// @brief assert function
/// @note this is the assert function with a return value. This function will not end the execution !!!!!
/// @note this function should not be use for critical part of the code but only for return code during execution issues 
/// @param p_pFileName : file name
/// @param p_uiLine : line number
/// @param p_ptLog : log pointer
///////////////////////////////////
#define X_ASSERT_RETURN(expr, ret) \
    do { \
        if (!(expr)) { \
            return assertValueWithReturn((uint8_t *)__FILE__, __LINE__, NULL, ret); \
        } \
    } while(0)


//////////////////////////////////
/// bool : s_bIsLogToFile 
//////////////////////////////////
static bool s_bIsLogToFile = false;

//////////////////////////////////
/// bool : s_ptcLogFilePath 
//////////////////////////////////
static char* s_ptcLogFilePath = NULL;

//////////////////////////////////
///@brief  load log file path
///@param  p_ptcLogFilePath : log file path
/// @return true if success, false otherwise
/// @note   this function should be called before assert
/// @note if the log file path is not set, the log will be printed to the console
////////////////////////////////// 
bool loadLogFilePath(char* p_ptcLogFilePath);

//////////////////////////////////
///@brief  assert : assert function
///@param  p_pFileName : file name
/// @param  p_uiLine : line number
/// @param  p_ptLog : log pointer
//////////////////////////////////
void assert(uint8_t* p_pFileName, uint32_t p_uiLine, void* p_ptLog);

//////////////////////////////////
/// @brief assert function with return value
/// @param p_pFileName : file name
/// @param p_uiLine : line number
/// @param p_ptLog : log pointer
/// @param ret : return value
/// @return : ret value
//////////////////////////////////
int assertValueWithReturn(uint8_t * p_pFileName, uint32_t p_uiLine, void* p_ptLog, int ret);



#endif // ASSERT_ASSERT_H_
