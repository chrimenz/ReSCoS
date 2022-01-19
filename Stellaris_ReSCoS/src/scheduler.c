/**************************************************************************************************
  Filename:       scheduler.c
  Revised:        $Date: 2013-02-20 17:37:33 +0100 (Mi, 20 Feb 2013) $
  Revision:       $Revision: 54 $
  Author:         $Author: Menz $

  Description:    Functions for a cooperative scheduling of tasks.


**************************************************************************************************/


/*! @file scheduler.c */


#include "inc/scheduler.h"

static void vScheduler(void);

/*!
 * structure representing a task handle
 */
struct typTask
{
	/** unique ID for one Task */
	unsigned char ucID;
	/** Task state @see etypTaskStates */
	volatile enum etypTaskStates eTaskState;
	/** Task function */
	void (*vTaskFunc)(void);
	/** Task period ms */
	unsigned long ulTaskPeriod;
	/** next start time for taks, when blocked by time */
	unsigned long ulNextStartTime;
};

/**
 * This structure holds all task handles.
 */
struct typTaskList
{
	/** ID of the current active task.*/
	volatile unsigned char tidActiveTask;
	/** Number of Tasks created.*/
	unsigned char ucNumTasks;
	/** Array of Task Handles */
	struct typTask atTask[SCDL_MAX_NUM_TASKS];
} tTaskList;

static unsigned long system_ticks = 0;

/*! **********************************************************************************
 * @fn		ucCreateTask
 *
 * @brief	Create a Task. First task created has highest priority.
 *
 * @param	vTaskFunc the "TASK"
 *
 * 			ulPeriod period for cyclic calls in ms. SCDL_INF_PERIOD for single call.
 * 			
 * @return	TaskHandle-ID
 */
taskID_t tidCreateTask( void (*vTaskFunc)(void), unsigned long ulPeriod)
{
	static unsigned char ucInit = 0;
	struct typTask tTaskHandle;
	
	SCDL_ASSERT(ulPeriod <= SCDL_MAX_TASK_PERIOD || ulPeriod == SCDL_INF_PERIOD);
	
	if(!ucInit)
	{
		tTaskList.ucNumTasks = 0;
		tTaskList.tidActiveTask = SCDL_NA;
		ucInit = 1;
	}
	
	SCDL_ASSERT(tTaskList.ucNumTasks < SCDL_MAX_NUM_TASKS);
	
	tTaskHandle = tTaskList.atTask[tTaskList.ucNumTasks];
	tTaskHandle.ucID = tTaskList.ucNumTasks;
	tTaskHandle.vTaskFunc = vTaskFunc;
	tTaskHandle.ulTaskPeriod = ulPeriod;
	tTaskHandle.eTaskState = READY;
	tTaskHandle.ulNextStartTime = 0;
	
	tTaskList.atTask[tTaskList.ucNumTasks] = tTaskHandle;
	
	tTaskList.ucNumTasks += 1;
	
	return tTaskHandle.ucID;
}

/*! **********************************************************************************
 * @fn		vTaskSetState
 *
 * @brief	Set Task state
 *
 * @param	taskID unique TASK-ID
 *
 * 			eState OFF,	READY, (ACTIVE not allowed), BLOCKED
 *
 */
void vTaskSetState( taskID_t taskID, enum etypTaskStates eState)
{
	/* we have a cooperative scheduler, so directly setting to active is not allowed */
	SCDL_ASSERT(eState != ACTIVE);
	
	/* is id initialized */
	SCDL_ASSERT(taskID != SCDL_NA);

	if(taskID < tTaskList.ucNumTasks)
		tTaskList.atTask[taskID].eTaskState = eState;
}

/*! **********************************************************************************
 * @fn		vSwitchAllTasksOff
 *
 * @brief	set states of each task to OFF, be careful that you activate at least one task after this call to prevent a dead lock
 *
 *
 */
void vSwitchAllTasksOff( void )
{
	unsigned short i;
	for (i = 0; i < tTaskList.ucNumTasks; i++)
	{
		tTaskList.atTask[i].eTaskState = OFF;
	}
}

/*! **********************************************************************************
 * @fn		vTaskSetPeriod
 *
 * @brief	Set Task period in ms
 *
 * @param	taskID unique TASK-ID
 *
 * 			ulPeriod period in ms, SCDL_INF_PERIOD for single call
 *
 */
void vTaskSetPeriod( taskID_t taskID, unsigned long ulPeriod)
{
	/* check if ID is okay */
	SCDL_ASSERT(taskID < tTaskList.ucNumTasks);

	tTaskList.atTask[taskID].ulTaskPeriod = ulPeriod;
}

/*! **********************************************************************************
 * @fn		vTaskInvokeDelayed
 *
 * @brief	Start task later
 *
 * @param	taskID unique TASK-ID
 *
 * 			delay in ms
 *
 */
void vTaskInvokeDelayed( taskID_t taskID, unsigned long ulDelay)
{

	unsigned long ulNextStart;

	/* check if ID is okay */
	SCDL_ASSERT(taskID < tTaskList.ucNumTasks);
	/* check possible overflow */
	SCDL_ASSERT(ulDelay < (0xFFFFFFFF - SCDL_MAX_SYSTICKS));

	ulNextStart = system_ticks + ulDelay;
	/* if we will have a wrap around in global systime, correct next start time */
	if(ulNextStart > SCDL_MAX_SYSTICKS)
		ulNextStart -= SCDL_MAX_SYSTICKS;

	if(taskID < tTaskList.ucNumTasks)
	{
		tTaskList.atTask[taskID].ulNextStartTime = ulNextStart;
		/* switch on if not active yet... */
		if(tTaskList.atTask[taskID].eTaskState == OFF)
			tTaskList.atTask[taskID].eTaskState = BLOCKED;
	}
}

static void vScheduler(void)
{
	struct typTask *ptTaskHandle;
	taskID_t tidActiveTaskID = SCDL_NA;
	taskID_t tidReadyTaskID = SCDL_NA;
	unsigned char i;
	volatile char numTasks;//TODO unsigned statt volatile?!
	
	numTasks = tTaskList.ucNumTasks;
	
	//is there a blocked task going to be ready?
	for(i = 0; i < numTasks; i++)
	{
		//tTaskHandle = tTaskList.atTask[i];
		ptTaskHandle = &tTaskList.atTask[i];
		
		/* check if a blocked task ready to start? */
		if(ptTaskHandle->eTaskState == BLOCKED)
		{
			/* current time reached nextstarttime */
			if( ptTaskHandle->ulNextStartTime <= system_ticks )
			{	
				ptTaskHandle->eTaskState = READY;
				//only if we had a wrap around, the nextstarttime is lower than the period
				if(ptTaskHandle->ulNextStartTime < ptTaskHandle->ulTaskPeriod)
				{
					ptTaskHandle->eTaskState = BLOCKED;
					/* but after the sysclock wrap we can start*/
					if(system_ticks < ptTaskHandle->ulTaskPeriod)
						ptTaskHandle->eTaskState = READY;
				}
			}
		}
		
		/* is there an active task, dont check if we already found one*/
		if(tidActiveTaskID == SCDL_NA)
		{
			if(ptTaskHandle->eTaskState == ACTIVE)
			{
				tidActiveTaskID = ptTaskHandle->ucID;
				/* TODO could return from scheduler here, because we have an active task -> nothing changes */
				//return;
			}
		}
		/* is there a ready task, dont check if we already found one*/
		if(tidReadyTaskID == SCDL_NA)
		{
			if(ptTaskHandle->eTaskState == READY)
			{
				tidReadyTaskID = ptTaskHandle->ucID;
			}
		}
		//tTaskList.atTask[i] = tTaskHandle;
	}



	if(tidActiveTaskID != SCDL_NA)/* there is an active task */
	{
		/* do nothing */
	}
	else if(tidReadyTaskID != SCDL_NA)/* there is a ready task */
	{
		/* switch to active --> start Task*/
		tTaskList.tidActiveTask = tidReadyTaskID;
		/* set pointer on active task handle */
		ptTaskHandle = &tTaskList.atTask[tidReadyTaskID];
		/* set state to active*/
		ptTaskHandle->eTaskState = ACTIVE;
		/* check and set the next start time */
		if( ptTaskHandle->ulTaskPeriod == SCDL_INF_PERIOD) /* we have a non periodic task */
				ptTaskHandle->ulNextStartTime = SCDL_INF_PERIOD;
		else{ /* we have periodic task */
			if (system_ticks + ptTaskHandle->ulTaskPeriod <= SCDL_MAX_SYSTICKS)
			{
				ptTaskHandle->ulNextStartTime = system_ticks + ptTaskHandle->ulTaskPeriod;
			}
			else
			{
//				ucVCOM_LogString("\nwrap\n",6);
				/* we'll have a wrap around */
				ptTaskHandle->ulNextStartTime = ptTaskHandle->ulTaskPeriod - (SCDL_MAX_SYSTICKS - system_ticks);
			}
		}
	}
	else
	{
		/* do nothing */
		tTaskList.tidActiveTask = SCDL_NA;
	}
}

/*! **********************************************************************************
 * @fn		vStartScheduler
 *
 * @brief	start the scheduler -> does not return!
 *
 */
void vStartScheduler(void)
{
	volatile unsigned char tidActiveTask = SCDL_NA;
	unsigned char bIdle = 0;
	
	for(;;)
	{
		tidActiveTask = tTaskList.tidActiveTask;

		/* check if theres an active task or not*/
		if(tidActiveTask == SCDL_NA)
		{
			/* we are not in idle mode yet -> call TaskHookFcn */
			if(!bIdle)
				SCDL_ON_TASK_START(tidActiveTask,system_ticks);

			/* set idle flag*/
			bIdle = 1;
		}
		else
		{

			//idle check test
			if(bIdle)
				SCDL_ON_TASK_STOP(tidActiveTask,system_ticks&0x0FFFF);

			/* reset idle flag */
			bIdle = 0;

			/* TaskStartMakro */
			//SCDL_ON_TASK_START(tidActiveTask,system_ticks);

			/* call task function */
			tTaskList.atTask[tidActiveTask].vTaskFunc();

			/* critical, because Scheduler call from Tick-ISR could occur */
			_disable_interrupts();

			/* after funcall set back to blocked, if still active (could be changed from inside) */
			if(tTaskList.atTask[tidActiveTask].eTaskState == ACTIVE)
				tTaskList.atTask[tidActiveTask].eTaskState = BLOCKED;

			/* we finished a task so lets invoke the scheduler manually to fill the gap until the next tick */
			vScheduler();

			_enable_interrupts();


			/* we set the active flag to n.a., so the task can be restarted if its reactivated by the scheduler */
			//tidActiveTask = SCDL_NA; TODO commented out cause supposed to be useless, test it!


		}

		/* TaskStopMakro */
		//SCDL_ON_TASK_STOP(tidActiveTask,system_ticks&0x0FFFF);
	}

}

/*! **********************************************************************************
 * @fn		vScdlTick1ms
 *
 * @brief	This function must be called from a systick interrupt every 1 ms
 *
 */
void vScdlTick1ms(void)
{
	system_ticks = (system_ticks < SCDL_MAX_SYSTICKS) ? system_ticks + 1 : 0;
	vScheduler();
}

unsigned char bSemaTake(sema_t *sema)
{
	if((*sema)){
		(*sema) = 0;
		return 1;
	}
	else
		return 0;
}

unsigned char bSemaCntTake(sema_t *sema)
{
	if((*sema) > 0){
		(*sema) -= 1;	
		return 1;
	}
	else
		return 0;
}
