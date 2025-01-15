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

    // Double-checked locking pattern
    if (!s_bInitialized)
    {
        return XOS_LOG_NOT_INIT;
    }

    int l_iRet = mutexLock(&s_tLogMutex);
    if (l_iRet != MUTEX_OK)
    {
        return XOS_LOG_ERROR;
    }

    // Vérifier à nouveau après avoir acquis le mutex
    if (!s_bInitialized)
    {
        mutexUnlock(&s_tLogMutex);
        return XOS_LOG_NOT_INIT;
    }

    // Formater le message
    va_list args;
    va_start(args, p_ptkcFormat);
    
    char l_cBuffer[XOS_LOG_MSG_SIZE];
    vsnprintf(l_cBuffer, sizeof(l_cBuffer), p_ptkcFormat, args);
    
    va_end(args);

    // Écrire le log dans le fichier
    if (s_tLogConfig.t_bLogToConsole)
    {
        printf("%s | %s:%d | %s\n", xHorodateurGetString(), p_ptkcFile, p_ulLine, l_cBuffer);
    }

    if (s_tLogConfig.t_bLogToFile)
    {
        FILE* l_ptFile = fopen(s_tLogConfig.t_cLogPath, "a");
        if (l_ptFile == NULL)
        {
            mutexUnlock(&s_tLogMutex);
            return XOS_LOG_ERROR;
        }

        fprintf(l_ptFile, "%s | %s:%d | %s\n", xHorodateurGetString(), p_ptkcFile, p_ulLine, l_cBuffer);
        fclose(l_ptFile);
    }

    mutexUnlock(&s_tLogMutex);
    return l_iRet;
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
