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
#include <signal.h>  

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
        p_pttOSTask->stack_size == 0) 
    {
        return OS_TASK_ERROR;
    }

    // Check priority values
#ifdef OS_USE_RT_SCHEDULING
    if (p_pttOSTask->priority < OS_TASK_LOWEST_PRIORITY || 
        p_pttOSTask->priority > OS_TASK_HIGHEST_PRIORITY) 
    {
        return OS_TASK_ERROR;
    }
#endif

    pthread_attr_t l_Attr;
    if (pthread_attr_init(&l_Attr) != 0) 
    {
        return OS_TASK_ERROR;
    }

    // Configure the thread attributes
    if (pthread_attr_setdetachstate(&l_Attr, PTHREAD_CREATE_JOINABLE) != 0) 
    {
        pthread_attr_destroy(&l_Attr);
        return OS_TASK_ERROR;
    }

    if (pthread_attr_setstacksize(&l_Attr, p_pttOSTask->stack_size) != 0) 
    {
        pthread_attr_destroy(&l_Attr);
        return OS_TASK_ERROR;
    }

    // Configure the priority and scheduling policy
#ifdef OS_USE_RT_SCHEDULING
    // For systems supporting real-time
    int policy;
    switch (p_pttOSTask->policy) {
        case OS_SCHED_FIFO:  policy = SCHED_FIFO; break;
        case OS_SCHED_RR:    policy = SCHED_RR; break;
        case OS_SCHED_BATCH: policy = SCHED_BATCH; break;
        case OS_SCHED_IDLE:  policy = SCHED_IDLE; break;
        default:             policy = SCHED_OTHER; break;
    }
    
    if (pthread_attr_setschedpolicy(&l_Attr, policy) != 0) 
    {
        pthread_attr_destroy(&l_Attr);
        return OS_TASK_ERROR;
    }
    
    p_pttOSTask->sched_param.sched_priority = p_pttOSTask->priority;
    if (pthread_attr_setschedparam(&l_Attr, &p_pttOSTask->sched_param) != 0) 
    {
        pthread_attr_destroy(&l_Attr);
        return OS_TASK_ERROR;
    }
    
    // Ensure that the scheduling attributes are respected
    if (pthread_attr_setinheritsched(&l_Attr, PTHREAD_EXPLICIT_SCHED) != 0) 
    {
        pthread_attr_destroy(&l_Attr);
        return OS_TASK_ERROR;
    }
#endif

    // Create the thread
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
    if (p_pttOSTask == NULL) 
    {
        return OS_TASK_ERROR;
    }

    if (p_pttOSTask->status == OS_TASK_STATUS_TERMINATED) 
    {
        return OS_TASK_SUCCESS; // Already terminated, no error
    }

    // Check if the thread is still valid
    if (pthread_kill(p_pttOSTask->handle, 0) != 0) 
    {
        // Thread already terminated
        p_pttOSTask->status = OS_TASK_STATUS_TERMINATED;
        return OS_TASK_SUCCESS;
    }

    // Request the thread cancellation
    int ret = pthread_cancel(p_pttOSTask->handle);
    if (ret != 0 && ret != ESRCH) { // ESRCH = No such process
        return OS_TASK_ERROR;
    }

    // Assign a timeout for the join to avoid blocking
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 2; // 2 seconds timeout

    ret = pthread_join(p_pttOSTask->handle, NULL);
    
    if (ret != 0 && ret != ESRCH) 
    {
        // If the join fails but the thread still exists
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
    
    // If the task is not yet terminated, check its current status
    if (p_pttOSTask->status != OS_TASK_STATUS_TERMINATED) 
    {
        // Check if the thread is still active
        int kill_result = pthread_kill(p_pttOSTask->handle, 0);
        
        // ESRCH means "No such process" - the thread no longer exists
        if (kill_result == ESRCH) 
        {
            p_pttOSTask->status = OS_TASK_STATUS_TERMINATED;
        }
        // Another error occurred during the check
        else if (kill_result != 0) 
        {
            p_pttOSTask->status = OS_TASK_STATUS_TERMINATED;
        }
        
        if (p_pttOSTask->status != OS_TASK_STATUS_TERMINATED) {
            
            // We can also check if the handle attribute is valid
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
    if (p_pttOSTask == NULL) 
    {
        return OS_TASK_ERROR;
    }

    // If the task is already terminated, no need to wait
    if (p_pttOSTask->status == OS_TASK_STATUS_TERMINATED) 
    {
        return OS_TASK_SUCCESS;
    }

    void* threadResult = NULL;
    int ret = pthread_join(p_pttOSTask->handle, &threadResult);
    
    if (ret != 0) 
    {
        return OS_TASK_ERROR;
    }
    
    p_pttOSTask->status = OS_TASK_STATUS_TERMINATED;
    
    if (threadResult == PTHREAD_CANCELED) {
        p_pttOSTask->exit_code = OS_TASK_EXIT_FAILURE;
    } 
    else 
    {
        p_pttOSTask->exit_code = (int)(intptr_t)threadResult;
    }
    
    if (p_pvExitValue != NULL) 
    {
        *p_pvExitValue = threadResult;
    }
    
    return OS_TASK_SUCCESS;
}
