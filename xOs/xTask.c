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
#else
	pthread_attr_t l_tAttr;
	pthread_attr_init(&l_tAttr);
	pthread_attr_setstacksize(&l_tAttr, tOSTask.stack_size);
	pthread_create(&tOSTask.handle, &l_tAttr, tOSTask.task, tOSTask.arg);
	pthread_attr_destroy(&l_tAttr);
#endif
	//check the task creation
	if (tOSTask.handle == NULL)
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

	//start the task
#ifdef _WIN32
	l_ulReturn = ResumeThread(tOSTask.handle);
#else
	l_ulReturn = osTaskResume(tOSTask);
#endif

	if (l_ulReturn == OS_TASK_ERROR)
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
	//check the task pointer
	X_ASSERT(p_pttOSTask != NULL);

	//convert to loccal structure
	os_task_t tOSTask = *p_pttOSTask;

	//check if the task is correctly started before interacting with it
	X_ASSERT(tOSTask.status != OS_TASK_STATUS_TERMINATED && tOSTask.status != OS_TASK_STATUS_BLOCKED);

	//check if the task is already suspended
	if (tOSTask.status == OS_TASK_STATUS_SUSPENDED)
	{
		return OS_TASK_SUCCESS;
	}

	unsigned long l_ulReturn = OS_TASK_ERROR;

	//suspend the task
#ifdef _WIN32
	l_ulReturn = SuspendThread(tOSTask.handle);
#else
	l_ulReturn = kill(tOSTask.id, SIGSTOP);
#endif
	if (l_ulReturn == OS_TASK_ERROR)
	{
		return OS_TASK_ERROR;
	}

	//set the task status
	tOSTask.status = OS_TASK_STATUS_SUSPENDED;

	//set the task pointer
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

	unsigned long l_ulReturn = OS_TASK_ERROR;

	//resume the task
#ifdef _WIN32
	l_ulReturn = ResumeThread(tOSTask.handle);
#else
	l_ulReturn = kill(tOSTask.id, SIGCONT);
#endif

	tOSTask.status = OS_TASK_STATUS_RUNNING;

	//set the task pointer
	*p_pttOSTask = tOSTask;

	return OS_TASK_SUCCESS;
}

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

	if (l_ulReturn == OS_TASK_ERROR)
	{
		return OS_TASK_ERROR;
	}

	//set the task status
	tOSTask.status = OS_TASK_STATUS_TERMINATED;

	//set the task pointer
	*p_pttOSTask = tOSTask;

	return OS_TASK_SUCCESS;
}

#ifdef _WIN32
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
	
	//update the task priority
	l_ulReturn = SetThreadPriority(tOSTask.handle, *p_ptiPriority);

	if (l_ulReturn == OS_TASK_ERROR)
	{
		return OS_TASK_ERROR;
	}

	tOSTask.priority = *p_ptiPriority;

	//set the task pointer
	*p_pttOSTask = tOSTask;

	return OS_TASK_SUCCESS;

}

#else
//////////////////////////////////////////////////////////////
/// osTaskUpdateParameters
//////////////////////////////////////////////////////////////
int osTaskUpdateParameters(os_task_t* p_pttOSTask, int* p_ptiPriority, int* p_ptiStackSize)
{
	X_ASSERT(p_pttOSTask != NULL);
	X_ASSERT(p_ptiPriority != NULL);
	X_ASSERT(p_ptiStackSize != NULL);
	//convert to local structure
	os_task_t tOSTask = *p_pttOSTask;

	struct sched_param l_tParam = {0};
	int l_iPolicy;
	int l_iReturn = OS_TASK_ERROR;

	//get the task policy
	l_iReturn = pthread_getschedparam(tOSTask.handle, &l_iPolicy, &l_tParam);
	if (l_iReturn != OS_TASK_SUCCESS)
	{
		return OS_TASK_ERROR;
	}

	//update the task priority
	l_tParam.sched_priority = *p_ptiPriority;
	l_iReturn = pthread_setschedparam(tOSTask.handle, l_iPolicy, &l_tParam);
	if (l_iReturn != OS_TASK_SUCCESS)
	{
		return OS_TASK_ERROR;
	}

	//update the task stack size
	l_iReturn = pthread_attr_setstacksize(&tOSTask.handle, *p_ptiStackSize);
	if (l_iReturn != OS_TASK_SUCCESS)
	{
		return OS_TASK_ERROR;
	}

	tOSTask.priority = *p_ptiPriority;
	tOSTask.stack_size = *p_ptiStackSize;

	//set the task pointer
	*p_pttOSTask = tOSTask;

	return OS_TASK_SUCCESS;
}

#endif

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
	pthread_join(tOSTask.handle, &tOSTask.exit_code);
#endif

	//set the task pointer
	* p_pttOSTask = tOSTask;

	return tOSTask.exit_code;
}
