////////////////////////////////////////////////////////////
// os Task src file
// defines the os function for task manipulation
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
// Intellectual property of Christophe Benedetti
////////////////////////////////////////////////////////////

#include "xTask.h"
#include <errno.h>
#include <signal.h>  // Pour pthread_kill

////////////////////////////////////////////////////////////
/// osTaskInit
////////////////////////////////////////////////////////////
int osTaskInit(t_TaskCtx* p_pttOSTask)
{
    if (p_pttOSTask == NULL) 
    {
        return OS_TASK_ERROR;
    }

    memset(p_pttOSTask, 0, sizeof(t_TaskCtx));
    p_pttOSTask->stack_size = OS_TASK_DEFAULT_STACK_SIZE;
    p_pttOSTask->priority = OS_TASK_DEFAULT_PRIORITY;
#ifdef OS_USE_RT_SCHEDULING
    p_pttOSTask->policy = OS_DEFAULT_SCHED_POLICY;
#endif
    p_pttOSTask->status = OS_TASK_STATUS_READY;
    p_pttOSTask->exit_code = OS_TASK_EXIT_SUCCESS;

    return OS_TASK_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osTaskCreate
////////////////////////////////////////////////////////////
int osTaskCreate(t_TaskCtx* p_pttOSTask)
{
    if (p_pttOSTask == NULL || p_pttOSTask->task == NULL ||
        p_pttOSTask->stack_size == 0) {
        return OS_TASK_ERROR;
    }

    // Vérification des valeurs de priorité
#ifdef OS_USE_RT_SCHEDULING
    if (p_pttOSTask->priority < OS_TASK_LOWEST_PRIORITY || 
        p_pttOSTask->priority > OS_TASK_HIGHEST_PRIORITY) {
        return OS_TASK_ERROR;
    }
#else
    if (p_pttOSTask->priority < OS_TASK_HIGHEST_PRIORITY || 
        p_pttOSTask->priority > OS_TASK_LOWEST_PRIORITY) {
        return OS_TASK_ERROR;
    }
#endif

    pthread_attr_t l_Attr;
    if (pthread_attr_init(&l_Attr) != 0) {
        return OS_TASK_ERROR;
    }

    // Configuration des attributs du thread
    if (pthread_attr_setdetachstate(&l_Attr, PTHREAD_CREATE_JOINABLE) != 0) {
        pthread_attr_destroy(&l_Attr);
        return OS_TASK_ERROR;
    }

    if (pthread_attr_setstacksize(&l_Attr, p_pttOSTask->stack_size) != 0) {
        pthread_attr_destroy(&l_Attr);
        return OS_TASK_ERROR;
    }

    // Configuration de la priorité et politique d'ordonnancement
#ifdef OS_USE_RT_SCHEDULING
    // Pour les systèmes supportant le temps réel
    int policy;
    switch (p_pttOSTask->policy) {
        case OS_SCHED_FIFO:  policy = SCHED_FIFO; break;
        case OS_SCHED_RR:    policy = SCHED_RR; break;
        case OS_SCHED_BATCH: policy = SCHED_BATCH; break;
        case OS_SCHED_IDLE:  policy = SCHED_IDLE; break;
        default:             policy = SCHED_OTHER; break;
    }
    
    if (pthread_attr_setschedpolicy(&l_Attr, policy) != 0) {
        pthread_attr_destroy(&l_Attr);
        return OS_TASK_ERROR;
    }
    
    p_pttOSTask->sched_param.sched_priority = p_pttOSTask->priority;
    if (pthread_attr_setschedparam(&l_Attr, &p_pttOSTask->sched_param) != 0) {
        pthread_attr_destroy(&l_Attr);
        return OS_TASK_ERROR;
    }
    
    // Assure que les attributs d'ordonnancement sont respectés
    if (pthread_attr_setinheritsched(&l_Attr, PTHREAD_EXPLICIT_SCHED) != 0) {
        pthread_attr_destroy(&l_Attr);
        return OS_TASK_ERROR;
    }
#endif

    // Création du thread
    int ret = pthread_create(&p_pttOSTask->handle, &l_Attr,
        p_pttOSTask->task, p_pttOSTask->arg);
    pthread_attr_destroy(&l_Attr);

    if (ret != 0) 
    {
        return OS_TASK_ERROR;
    }

    p_pttOSTask->status = OS_TASK_STATUS_RUNNING;
    p_pttOSTask->id = (int)(uintptr_t)p_pttOSTask->handle;

    return OS_TASK_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osTaskEnd
////////////////////////////////////////////////////////////
int osTaskEnd(t_TaskCtx* p_pttOSTask)
{
    if (p_pttOSTask == NULL) {
        return OS_TASK_ERROR;
    }

    if (p_pttOSTask->status == OS_TASK_STATUS_TERMINATED) {
        return OS_TASK_SUCCESS; // Déjà terminé, pas d'erreur
    }

    // Vérifie si le thread est encore valide
    if (pthread_kill(p_pttOSTask->handle, 0) != 0) {
        // Thread déjà terminé
        p_pttOSTask->status = OS_TASK_STATUS_TERMINATED;
        return OS_TASK_SUCCESS;
    }

    // Demande l'annulation du thread
    int ret = pthread_cancel(p_pttOSTask->handle);
    if (ret != 0 && ret != ESRCH) { // ESRCH = No such process
        return OS_TASK_ERROR;
    }

    // Attribue un timeout pour le join afin d'éviter un blocage
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 2; // 2 secondes de timeout

    ret = pthread_join(p_pttOSTask->handle, NULL);
    
    if (ret != 0 && ret != ESRCH) 
    {
        // Si le join échoue mais que le thread existe toujours
        p_pttOSTask->status = OS_TASK_STATUS_BLOCKED;
        return OS_TASK_ERROR;
    }

    p_pttOSTask->status = OS_TASK_STATUS_TERMINATED;
    return OS_TASK_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osTaskGetStatus
////////////////////////////////////////////////////////////
int osTaskGetStatus(t_TaskCtx* p_pttOSTask)
{
    if (p_pttOSTask == NULL) {
        return OS_TASK_ERROR;
    }
    
    // Si la tâche n'est pas encore terminée, vérifier son état actuel
    if (p_pttOSTask->status != OS_TASK_STATUS_TERMINATED) {
        // Vérifie si le thread est toujours actif
        int kill_result = pthread_kill(p_pttOSTask->handle, 0);
        
        // ESRCH signifie "No such process" - le thread n'existe plus
        if (kill_result == ESRCH) {
            p_pttOSTask->status = OS_TASK_STATUS_TERMINATED;
        }
        // Une autre erreur s'est produite lors de la vérification
        else if (kill_result != 0) 
        {

            p_pttOSTask->status = OS_TASK_STATUS_TERMINATED;
        }
        
        if (p_pttOSTask->status != OS_TASK_STATUS_TERMINATED) {
            
            // On peut aussi vérifier si l'attribut handle est valide
            if (p_pttOSTask->handle == 0 || p_pttOSTask->handle == (pthread_t)-1) 
            {
                p_pttOSTask->status = OS_TASK_STATUS_TERMINATED;
            }
        }
    }
    
    return p_pttOSTask->status;
}

////////////////////////////////////////////////////////////
/// osTaskWait
////////////////////////////////////////////////////////////
int osTaskWait(t_TaskCtx* p_pttOSTask, void** p_pvExitValue)
{
    if (p_pttOSTask == NULL) {
        return OS_TASK_ERROR;
    }

    // Si la tâche est déjà terminée, pas besoin d'attendre
    if (p_pttOSTask->status == OS_TASK_STATUS_TERMINATED) {
        return OS_TASK_SUCCESS;
    }

    void* threadResult = NULL;
    int ret = pthread_join(p_pttOSTask->handle, &threadResult);
    
    if (ret != 0) {
        return OS_TASK_ERROR;
    }
    
    p_pttOSTask->status = OS_TASK_STATUS_TERMINATED;
    
    if (threadResult == PTHREAD_CANCELED) {
        p_pttOSTask->exit_code = OS_TASK_EXIT_FAILURE;
    } else {
        p_pttOSTask->exit_code = (int)(intptr_t)threadResult;
    }
    
    if (p_pvExitValue != NULL) {
        *p_pvExitValue = threadResult;
    }
    
    return OS_TASK_SUCCESS;
}
