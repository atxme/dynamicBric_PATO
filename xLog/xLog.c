////////////////////////////////////////////////////////////
//  log source file
//  implements log functions
//
// general discloser: copy or share the file is forbidden
// Written : 12/01/2025
////////////////////////////////////////////////////////////

#include "xLog.h"
#include "xAssert.h"
#include "xOsHorodateur.h"
#include "xOsMutex.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static xos_log_config_t s_tLogConfig = { 0 };
static bool s_bInitialized = false;
static xos_mutex_t s_tLogMutex;

int xLogInit(xos_log_config_t* p_ptConfig)
{
    X_ASSERT(p_ptConfig != NULL);

    int l_iRet = mutexCreate(&s_tLogMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_LOG_ERROR;
    }

    memcpy(&s_tLogConfig, p_ptConfig, sizeof(xos_log_config_t));

    if (s_tLogConfig.t_bLogToFile)
    {
        mutexLock(&s_tLogMutex);
        FILE* l_ptFile = fopen(s_tLogConfig.t_cLogPath, "w");
        if (l_ptFile == NULL)
        {
            mutexUnlock(&s_tLogMutex);
            mutexDestroy(&s_tLogMutex);
            return XOS_LOG_ERROR;
        }
        fclose(l_ptFile);
        mutexUnlock(&s_tLogMutex);
    }

    s_bInitialized = true;
    return XOS_LOG_OK;
}

int xLogWrite(const char* p_ptkcFile, uint32_t p_ulLine, const char* p_ptkcFormat, ...)
{
    X_ASSERT(p_ptkcFile != NULL);
    X_ASSERT(p_ptkcFormat != NULL);

    int l_iRet = mutexLock(&s_tLogMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_LOG_ERROR;
    }

    if (!s_bInitialized)
    {
        mutexUnlock(&s_tLogMutex);
        return XOS_LOG_NOT_INIT;
    }
    mutexUnlock(&s_tLogMutex);

    char l_cMessage[XOS_LOG_MSG_SIZE / 2];
    va_list l_tArgs;
    va_start(l_tArgs, p_ptkcFormat);
    l_iRet = vsnprintf(l_cMessage, sizeof(l_cMessage), p_ptkcFormat, l_tArgs);
    va_end(l_tArgs);

    if (l_iRet < 0 || l_iRet >= sizeof(l_cMessage))
    {
        return XOS_LOG_ERROR;
    }

    const char* l_ptcTimestamp = xHorodateurGetString();
    if (l_ptcTimestamp == NULL)
    {
        return XOS_LOG_ERROR;
    }

    char l_cFinalMessage[XOS_LOG_MSG_SIZE];
    int l_iLen = snprintf(l_cFinalMessage, XOS_LOG_MSG_SIZE - 1,
        "%s | %s : %u | %s\n",
        l_ptcTimestamp, p_ptkcFile, p_ulLine, l_cMessage);

    if (l_iLen < 0 || l_iLen >= XOS_LOG_MSG_SIZE - 1)
    {
        return XOS_LOG_ERROR;
    }

    if (s_tLogConfig.t_bLogToConsole)
    {
        l_iRet = mutexLock(&s_tLogMutex);
        if (l_iRet != MUTEX_OK)
        {
            return XOS_LOG_ERROR;
        }
        
        if (printf("%s", l_cFinalMessage) < 0)
        {
            mutexUnlock(&s_tLogMutex);
            return XOS_LOG_ERROR;
        }
        
        mutexUnlock(&s_tLogMutex);
    }

    if (s_tLogConfig.t_bLogToFile)
    {
        l_iRet = mutexLock(&s_tLogMutex);
        if (l_iRet != MUTEX_OK)
        {
            return XOS_LOG_ERROR;
        }

        FILE* l_ptFile = fopen(s_tLogConfig.t_cLogPath, "a");
        if (l_ptFile == NULL)
        {
            mutexUnlock(&s_tLogMutex);
            return XOS_LOG_ERROR;
        }

        if (fprintf(l_ptFile, "%s", l_cFinalMessage) < 0)
        {
            fclose(l_ptFile);
            mutexUnlock(&s_tLogMutex);
            return XOS_LOG_ERROR;
        }

        if (fclose(l_ptFile) != 0)
        {
            mutexUnlock(&s_tLogMutex);
            return XOS_LOG_ERROR;
        }

        mutexUnlock(&s_tLogMutex);
    }

    return XOS_LOG_OK;
}

int xLogClose(void)
{
    int l_iRet = mutexLock(&s_tLogMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_LOG_ERROR;
    }

    if (s_bInitialized)
    {
        s_bInitialized = false;
        mutexUnlock(&s_tLogMutex);
        l_iRet = mutexDestroy(&s_tLogMutex);
        if (l_iRet != MUTEX_OK)
        {
            return XOS_LOG_ERROR;
        }
    }
    else
    {
        mutexUnlock(&s_tLogMutex);
    }

    return XOS_LOG_OK;
}
