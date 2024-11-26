////////////////////////////////////////////////////////////
//  os Task src file
//  defines the os function for task manipulation
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include "xTask.h"

//////////////////////////////////////////////////////////////
/// osTaskCreate
//////////////////////////////////////////////////////////////
int osTaskCreate(os_task_t* p_pttOSTask)
{
	//check the task pointer
	X_ASSERT(p_pttOSTask != NULL);

	//convert to loccal structure
	os_task_t tOSTask = *p_pttOSTask;

	//check the task function pointer 
	// we check only the parameters that are mandatory others will be custom by the function 
	X_ASSERT(tOSTask.task != NULL);
	X_ASSERT(tOSTask.priority >= OS_TASK_LOWEST_PRIORITY && tOSTask.priority <= OS_TASK_HIGHEST_PRIORITY);
	X_ASSERT(tOSTask.stack_size > 0);
	
	//create the task
#ifdef _WIN32
	tOSTask.handle = CreateThread(NULL, tOSTask.stack_size, tOSTask.task, tOSTask.arg, 0, NULL);
	tOSTask.id = GetThreadId(tOSTask.handle);
#else
	pthread_attr_t l_tAttr;
	pthread_attr_init(&l_tAttr);
	pthread_attr_setstacksize(&l_tAttr, tOSTask.stack_size);
	pthread_create(&tOSTask.handle, &l_tAttr, tOSTask.task, tOSTask.arg);
	pthread_attr_destroy(&l_tAttr);
#endif
	//check the task creation
	if (tOSTask.handle == 0)
	{
		return OS_TASK_ERROR;
	}

	//set the task status
	tOSTask.status = OS_TASK_STATUS_READY;

	//set the task id
	tOSTask.id = tOSTask.handle;

	//set the task exit code
	tOSTask.exit_code = OS_TASK_EXIT_SUCCESS;

	//set the task pointer
	*p_pttOSTask = tOSTask;

	return OS_TASK_SUCCESS;

}

#ifdef _WIN32
//////////////////////////////////////////////////////////////
/// osTaskStart
//////////////////////////////////////////////////////////////
int osTaskStart(os_task_t* p_pttOSTask)
{
	//check the task pointer
	X_ASSERT(p_pttOSTask != NULL);

	//convert to loccal structure
	os_task_t tOSTask = *p_pttOSTask;

	//check the task status
	X_ASSERT(tOSTask.status == OS_TASK_STATUS_READY);

	unsigned long l_ulReturn = OS_TASK_ERROR;

	l_ulReturn = ResumeThread(tOSTask.handle);


	if ((int)l_ulReturn == OS_TASK_ERROR)
	{
		return OS_TASK_ERROR;
	}

	//set the task status
	tOSTask.status = OS_TASK_STATUS_RUNNING;

	//set the task pointer
	*p_pttOSTask = tOSTask;

	return OS_TASK_SUCCESS;
}

//////////////////////////////////////////////////////////////
/// osTaskSuspend
//////////////////////////////////////////////////////////////
int osTaskSuspend(os_task_t* p_pttOSTask)
{
    // Vérification du pointeur de tâche
    X_ASSERT(p_pttOSTask != NULL);

    // Copie locale de la structure pour travailler en sécurité
    os_task_t tOSTask = *p_pttOSTask;

    // Vérification que la tâche est valide et peut être suspendue
    X_ASSERT(tOSTask.status != OS_TASK_STATUS_TERMINATED);
    X_ASSERT(tOSTask.status != OS_TASK_STATUS_BLOCKED);

    // Si la tâche est déjà suspendue, aucune action n'est nécessaire
    if (tOSTask.status == OS_TASK_STATUS_SUSPENDED) {
        return OS_TASK_SUCCESS;
    }

    int l_ulReturn = OS_TASK_ERROR;


    if (tOSTask.handle != NULL) {
        l_ulReturn = SuspendThread(tOSTask.handle);
    } else {
        return OS_TASK_ERROR; // Handle invalide
    }


    // Vérifier si la suspension a réussi
    if (l_ulReturn != 0) {
        return OS_TASK_ERROR;
    }

    // Mise à jour de l'état de la tâche
    tOSTask.status = OS_TASK_STATUS_SUSPENDED;

    // Mise à jour du pointeur d'entrée
    *p_pttOSTask = tOSTask;

    return OS_TASK_SUCCESS;
}

//////////////////////////////////////////////////////////////
/// osTaskResume
//////////////////////////////////////////////////////////////
int osTaskResume(os_task_t* p_pttOSTask)
{
	//check the task pointer
	X_ASSERT(p_pttOSTask != NULL);

	//convert to loccal structure
	os_task_t tOSTask = *p_pttOSTask;

	//check if the task is correctly started before interacting with it
	X_ASSERT(tOSTask.status != OS_TASK_STATUS_TERMINATED && tOSTask.status != OS_TASK_STATUS_BLOCKED);

	//check if the task is already running
	if (tOSTask.status == OS_TASK_STATUS_RUNNING)
	{
		return OS_TASK_SUCCESS;
	}

	unsigned long l_ulReturn = (unsigned long)OS_TASK_ERROR;

	l_ulReturn = ResumeThread(tOSTask.handle);

	tOSTask.status = OS_TASK_STATUS_RUNNING;

	//set the task pointer
	*p_pttOSTask = tOSTask;

	return OS_TASK_SUCCESS;
}
#endif 

//////////////////////////////////////////////////////////////
/// osTaskEnd
//////////////////////////////////////////////////////////////
int osTaskEnd(os_task_t* p_pttOSTask)
{
	//check the task pointer
	X_ASSERT(p_pttOSTask != NULL);

	//convert to loccal structure
	os_task_t tOSTask = *p_pttOSTask;

	//check if the task is correctly started before interacting with it
	X_ASSERT(tOSTask.status != OS_TASK_STATUS_TERMINATED);

	unsigned long l_ulReturn = OS_TASK_ERROR;

	//end the task
#ifdef _WIN32
	l_ulReturn = TerminateThread(tOSTask.handle, tOSTask.exit_code);

#else
	l_ulReturn = pthread_cancel(tOSTask.handle);
#endif

	if ((int)l_ulReturn == OS_TASK_ERROR)
	{
		return OS_TASK_ERROR;
	}

	//set the task status
	tOSTask.status = OS_TASK_STATUS_TERMINATED;

	//set the task pointer
	*p_pttOSTask = tOSTask;

	return OS_TASK_SUCCESS;
}

//////////////////////////////////////////////////////////////
/// osTaskUpdateParameters
//////////////////////////////////////////////////////////////
int osTaskUpdatePriority(os_task_t* p_pttOSTask, int* p_ptiPriority)
{
	X_ASSERT(p_pttOSTask != NULL);
	X_ASSERT(p_ptiPriority != NULL);
	//convert to local structure
	os_task_t tOSTask = *p_pttOSTask;

	unsigned long l_ulReturn = OS_TASK_ERROR;
#ifdef _WIN32
	//update the task priority
	l_ulReturn = SetThreadPriority(tOSTask.handle, *p_ptiPriority);
#else
	struct sched_param l_tParam;
	l_tParam.sched_priority = *p_ptiPriority;
	//update the task priority
	l_ulReturn = pthread_setschedparam(tOSTask.handle, SCHED_RR, &l_tParam);
#endif


	if ((int)l_ulReturn == OS_TASK_ERROR)
	{
		return OS_TASK_ERROR;
	}

	tOSTask.priority = *p_ptiPriority;

	//set the task pointer
	*p_pttOSTask = tOSTask;

	return OS_TASK_SUCCESS;

}

//////////////////////////////////////////////////////////////
/// osTaskGetExitCode
//////////////////////////////////////////////////////////////
int osTaskGetExitCode(os_task_t* p_pttOSTask)
{
	//check the task pointer
	X_ASSERT(p_pttOSTask != NULL);

	//convert to loccal structure
	os_task_t tOSTask = *p_pttOSTask;

#ifdef _WIN32
	DWORD l_dwExitCode = 0;
	//get the task exit code
	GetExitCodeThread(tOSTask.handle, &l_dwExitCode);
	tOSTask.exit_code = l_dwExitCode;
#else
	//get the task exit code
	pthread_join(tOSTask.handle, (void*)&tOSTask.exit_code);
#endif

	//set the task pointer
	* p_pttOSTask = tOSTask;

	return tOSTask.exit_code;
}
