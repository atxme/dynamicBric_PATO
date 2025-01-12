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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static xos_log_config_t s_tLogConfig = { 0 };
static bool s_bInitialized = false;

int xLogInit(xos_log_config_t* p_ptConfig)
{
    X_ASSERT(p_ptConfig != NULL);

    memcpy(&s_tLogConfig, p_ptConfig, sizeof(xos_log_config_t));

    if (s_tLogConfig.t_bLogToFile)
    {
        FILE* l_ptFile = fopen(s_tLogConfig.t_cLogPath, "w");
        if (l_ptFile == NULL)
        {
            return XOS_LOG_ERROR;
        }
        fclose(l_ptFile);
    }

    s_bInitialized = true;
    return XOS_LOG_OK;
}

int xLogWrite(const char* p_ptkcFile, uint32_t p_ulLine, const char* p_ptkcFormat, ...)
{
    X_ASSERT(p_ptkcFile != NULL);
    X_ASSERT(p_ptkcFormat != NULL);

    if (!s_bInitialized)
    {
        return XOS_LOG_NOT_INIT;
    }

    // Format le message utilisateur
    char l_cMessage[XOS_LOG_MSG_SIZE / 2];
    va_list l_tArgs;
    va_start(l_tArgs, p_ptkcFormat);
    vsnprintf(l_cMessage, sizeof(l_cMessage), p_ptkcFormat, l_tArgs);
    va_end(l_tArgs);

    // Récupère l'horodatage
    const char* l_ptcTimestamp = xHorodateurGet();
    if (l_ptcTimestamp == NULL)
    {
        return XOS_LOG_ERROR;
    }

    // Prépare le message final
    char l_cFinalMessage[XOS_LOG_MSG_SIZE];
    int l_iLen = snprintf(l_cFinalMessage, XOS_LOG_MSG_SIZE - 1,
        "%s | %s : %u | %s\n",
        l_ptcTimestamp, p_ptkcFile, p_ulLine, l_cMessage);

    if (l_iLen < 0 || l_iLen >= XOS_LOG_MSG_SIZE - 1)
    {
        return XOS_LOG_ERROR;
    }

    // Écrit sur la console si activé
    if (s_tLogConfig.t_bLogToConsole)
    {
        printf("%s", l_cFinalMessage);
    }

    // Écrit dans le fichier si activé
    if (s_tLogConfig.t_bLogToFile)
    {
        FILE* l_ptFile = fopen(s_tLogConfig.t_cLogPath, "a");
        if (l_ptFile == NULL)
        {
            return XOS_LOG_ERROR;
        }
        fprintf(l_ptFile, "%s", l_cFinalMessage);
        fclose(l_ptFile);
    }

    return XOS_LOG_OK;
}

int xLogClose(void)
{
    s_bInitialized = false;
    return XOS_LOG_OK;
}
