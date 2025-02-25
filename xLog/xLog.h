////////////////////////////////////////////////////////////
//  log header file
//  defines log types and functions
//
// general discloser: copy or share the file is forbidden
// Written : 12/01/2025
////////////////////////////////////////////////////////////
#pragma once

#ifndef XOS_LOG_H_
#define XOS_LOG_H_

#include <stdint.h>
#include <stdbool.h>

// Log error codes
#define XOS_LOG_OK            0
#define XOS_LOG_ERROR        -1
#define XOS_LOG_INVALID      -2
#define XOS_LOG_NOT_INIT     -3
#define XOS_LOG_MUTEX_ERROR  -4

// Log buffer sizes
#define XOS_LOG_PATH_SIZE    256
#define XOS_LOG_MSG_SIZE     1024

// Log configuration structure
typedef struct
{
    bool t_bLogToFile;            // Enable file logging
    bool t_bLogToConsole;         // Enable console logging
    char t_cLogPath[XOS_LOG_PATH_SIZE]; // Log file path
} t_logCtx;

//////////////////////////////////
/// @brief Initialize logging system
/// @param p_ptConfig : log configuration
/// @return success or error code
//////////////////////////////////
int xLogInit(t_logCtx* p_ptConfig);

//////////////////////////////////
/// @brief Write log message
/// @param p_ptkcFile : source file
/// @param p_ulLine : line number
/// @param p_ptkcFormat : message format
/// @return success or error code
//////////////////////////////////
int xLogWrite(const char* p_ptkcFile, uint32_t p_ulLine, const char* p_ptkcFormat, ...);

//////////////////////////////////
/// @brief Close logging system
/// @return success or error code
//////////////////////////////////
int xLogClose(void);

// Log macros
#define X_LOG_TRACE(msg, ...) xLogWrite(__FILE__, __LINE__, "TRACE | " msg, ##__VA_ARGS__)
#define X_LOG_ASSERT(msg, ...) xLogWrite(__FILE__, __LINE__, "ASSERT | " msg, ##__VA_ARGS__)

#endif // XOS_LOG_H_
