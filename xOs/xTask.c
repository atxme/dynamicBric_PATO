////////////////////////////////////////////////////////////
// os Task src file
// defines the os function for task manipulation
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include "xTask.h"
#include <errno.h>

////////////////////////////////////////////////////////////
/// osTaskCreate
////////////////////////////////////////////////////////////
int osTaskCreate(t_TaskCtx* p_pttOSTask)
{
    if (p_pttOSTask == NULL || p_pttOSTask->task == NULL ||
        p_pttOSTask->stack_size == 0) {
        return OS_TASK_ERROR;
    }

    pthread_attr_t attr;
    struct sched_param schedParam;

    if (pthread_attr_init(&attr) != 0) {
        return OS_TASK_ERROR;
    }

    // Configuration des attributs du thread
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize(&attr, p_pttOSTask->stack_size);

	// Configuration de la priorité
#ifdef OS_USE_RT_SCHEDULING
    // Configuration de la politique d'ordonnancement
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    schedParam.sched_priority = p_pttOSTask->priority;
    pthread_attr_setschedparam(&attr, &schedParam);
#endif

    // Création du thread
    int ret = pthread_create(&p_pttOSTask->handle, &attr,
        p_pttOSTask->task, p_pttOSTask->arg);
    pthread_attr_destroy(&attr);

    if (ret != 0) {
        return OS_TASK_ERROR;
    }

    p_pttOSTask->status = OS_TASK_STATUS_READY;
    p_pttOSTask->id = (int)(uintptr_t)p_pttOSTask->handle;
    p_pttOSTask->exit_code = OS_TASK_EXIT_SUCCESS;

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
        return OS_TASK_ERROR;
    }

    int ret = pthread_cancel(p_pttOSTask->handle);
    if (ret != 0) {
        return OS_TASK_ERROR;
    }

    // Jointure pour libérer les ressources
    if (pthread_join(p_pttOSTask->handle, NULL) != 0) {
        return OS_TASK_ERROR;
    }

    p_pttOSTask->status = OS_TASK_STATUS_TERMINATED;
    return OS_TASK_SUCCESS;
}

////////////////////////////////////////////////////////////
/// osTaskGetExitCode
////////////////////////////////////////////////////////////
int osTaskGetExitCode(t_TaskCtx* p_pttOSTask)
{
    if (p_pttOSTask == NULL) {
        return OS_TASK_ERROR;
    }

    if (p_pttOSTask->status != OS_TASK_STATUS_TERMINATED) {
        void* threadResult = NULL;
        if (pthread_join(p_pttOSTask->handle, &threadResult) != 0) {
            return OS_TASK_ERROR;
        }
        p_pttOSTask->exit_code = (int)(intptr_t)threadResult;
    }
    return p_pttOSTask->exit_code;
}
