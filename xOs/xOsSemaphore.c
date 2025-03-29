////////////////////////////////////////////////////////////
// os Semaphore src file
// defines the os function for semaphore manipulation
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

// Ces définitions sont déjà incluses dans les options de compilation du projet
// (CMakeLists.txt), donc nous n'avons pas besoin de les redéfinir ici

#include "xOsSemaphore.h"
#include <time.h>  // Pour CLOCK_REALTIME

////////////////////////////////////////////////////////////
/// osSemInit
////////////////////////////////////////////////////////////
int osSemInit(t_OSSemCtx* p_pttOSSem, int p_iInitValue, const char* p_pcName)
{
    if (p_pttOSSem == NULL || p_iInitValue < 0) {
        return OS_SEM_ERROR;
    }

    // Réinitialiser la structure
    memset(p_pttOSSem, 0, sizeof(t_OSSemCtx));
    
    p_pttOSSem->value = p_iInitValue;
    p_pttOSSem->name = p_pcName;
    
    // Création du sémaphore
    if (p_pcName != NULL) {
        // Sémaphore nommé
        sem_t* namedSem = sem_open(p_pcName, O_CREAT, 0666, p_iInitValue);
        if (namedSem == SEM_FAILED) {
            return OS_SEM_ERROR;
        }
        // Pour les sémaphores nommés, on stocke le pointeur retourné par sem_open
        p_pttOSSem->sem_handle = namedSem;
        p_pttOSSem->is_named = 1;
    } else {
        // Sémaphore anonyme
        if (sem_init(&p_pttOSSem->sem, 0, p_iInitValue) != 0) {
            return OS_SEM_ERROR;
        }
        p_pttOSSem->is_named = 0;
    }
    
    p_pttOSSem->initialized = 1;
    return OS_SEM_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osSemDestroy
////////////////////////////////////////////////////////////
int osSemDestroy(t_OSSemCtx* p_pttOSSem)
{
    if (p_pttOSSem == NULL || !p_pttOSSem->initialized) {
        return OS_SEM_ERROR;
    }
    
    int result;
    if (p_pttOSSem->is_named) {
        // Sémaphore nommé
        result = sem_close(p_pttOSSem->sem_handle);
        if (result == 0 && p_pttOSSem->name != NULL) {
            // Suppression du sémaphore nommé du système si nous sommes le dernier utilisateur
            sem_unlink(p_pttOSSem->name);
        }
    } else {
        // Sémaphore anonyme
        result = sem_destroy(&p_pttOSSem->sem);
    }
    
    if (result != 0) {
        return OS_SEM_ERROR;
    }
    
    p_pttOSSem->initialized = 0;
    return OS_SEM_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osSemWait
////////////////////////////////////////////////////////////
int osSemWait(t_OSSemCtx* p_pttOSSem)
{
    if (p_pttOSSem == NULL || !p_pttOSSem->initialized) {
        return OS_SEM_ERROR;
    }
    
    int result;
    if (p_pttOSSem->is_named) {
        result = sem_wait(p_pttOSSem->sem_handle);
    } else {
        result = sem_wait(&p_pttOSSem->sem);
    }
    
    if (result != 0) {
        return OS_SEM_ERROR;
    }
    
    // Mettre à jour la valeur (approximative, car il peut y avoir des accès concurrents)
    osSemGetValue(p_pttOSSem, &p_pttOSSem->value);
    
    return OS_SEM_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osSemWaitTimeout
////////////////////////////////////////////////////////////
int osSemWaitTimeout(t_OSSemCtx* p_pttOSSem, unsigned long p_ulTimeoutMs)
{
    if (p_pttOSSem == NULL || !p_pttOSSem->initialized) {
        return OS_SEM_ERROR;
    }
    
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        return OS_SEM_ERROR;
    }
    
    // Convertir millisecondes en secondes et nanosecondes
    ts.tv_sec += p_ulTimeoutMs / 1000;
    ts.tv_nsec += (p_ulTimeoutMs % 1000) * 1000000;
    
    // Normaliser les nanosecondes (assurer qu'elles restent < 1 seconde)
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }
    
    int ret;
    if (p_pttOSSem->is_named) {
        ret = sem_timedwait(p_pttOSSem->sem_handle, &ts);
    } else {
        ret = sem_timedwait(&p_pttOSSem->sem, &ts);
    }
    
    if (ret != 0) {
        if (errno == ETIMEDOUT) {
            return OS_SEM_TIMEOUT;
        }
        return OS_SEM_ERROR;
    }
    
    // Mettre à jour la valeur (approximative)
    osSemGetValue(p_pttOSSem, &p_pttOSSem->value);
    
    return OS_SEM_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osSemTryWait
////////////////////////////////////////////////////////////
int osSemTryWait(t_OSSemCtx* p_pttOSSem)
{
    if (p_pttOSSem == NULL || !p_pttOSSem->initialized) {
        return OS_SEM_ERROR;
    }
    
    int ret;
    if (p_pttOSSem->is_named) {
        ret = sem_trywait(p_pttOSSem->sem_handle);
    } else {
        ret = sem_trywait(&p_pttOSSem->sem);
    }
    
    if (ret != 0) {
        if (errno == EAGAIN) {
            return OS_SEM_NOT_AVAILABLE;
        }
        return OS_SEM_ERROR;
    }
    
    // Mettre à jour la valeur (approximative)
    osSemGetValue(p_pttOSSem, &p_pttOSSem->value);
    
    return OS_SEM_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osSemPost
////////////////////////////////////////////////////////////
int osSemPost(t_OSSemCtx* p_pttOSSem)
{
    if (p_pttOSSem == NULL || !p_pttOSSem->initialized) {
        return OS_SEM_ERROR;
    }
    
    int result;
    if (p_pttOSSem->is_named) {
        result = sem_post(p_pttOSSem->sem_handle);
    } else {
        result = sem_post(&p_pttOSSem->sem);
    }
    
    if (result != 0) {
        return OS_SEM_ERROR;
    }
    
    // Mettre à jour la valeur (approximative)
    osSemGetValue(p_pttOSSem, &p_pttOSSem->value);
    
    return OS_SEM_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osSemGetValue
////////////////////////////////////////////////////////////
int osSemGetValue(t_OSSemCtx* p_pttOSSem, int* p_piValue)
{
    if (p_pttOSSem == NULL || !p_pttOSSem->initialized || p_piValue == NULL) {
        return OS_SEM_ERROR;
    }
    
    int result;
    if (p_pttOSSem->is_named) {
        result = sem_getvalue(p_pttOSSem->sem_handle, p_piValue);
    } else {
        result = sem_getvalue(&p_pttOSSem->sem, p_piValue);
    }
    
    if (result != 0) {
        return OS_SEM_ERROR;
    }
    
    return OS_SEM_SUCCESS;
}
