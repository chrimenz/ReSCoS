#ifndef SCHEDULER_H_
#define SCHEDULER_H_

/*! @file */

#define SCDL_MAX_SYSTICKS		(0x7FFFFFFF)
#define SCDL_MAX_TASK_PERIOD	SCDL_MAX_SYSTICKS
#define SCDL_MAX_NUM_TASKS		(12)
#define SCDL_INF_PERIOD			(0xFFFFFFFF)
#define SCDL_NA					(0xFF)	
#if	(SCDL_MAX_NUM_TASKS > SCDL_NA)
#error SCDL_MAX_NUM_TASKS > SCDL_NA
#endif


#define SCDL_ASSERTS_ON
#ifdef SCDL_ASSERTS_ON
#define SCDL_ASSERT(x)	if(!(x))  while(1);
#else
#define SCDL_ASSERT(x)	{ }
#endif

typedef unsigned char sema_t;
typedef unsigned char taskID_t;

#define SEMAPHORE_TAKE(s)	bSemaTake(&(s))
#define SEMAPHORE_GIVE(s)	((s)=1)

#define SEMAPHORE_CNT_GIVE(s)	((s)+=1)
#define SEMAPHORE_CNT_TAKE(s)	bSemaCntTake(&(s))

enum etypTaskStates{
	OFF = 0,
	READY,
	ACTIVE,
	BLOCKED
};

//#define SCDL_USE_TASK_HOOKS
#ifdef SCDL_USE_TASK_HOOKS
extern unsigned long g_ulSytemTime;
#define SCDL_ON_TASK_START(id,t)	 	{ 	uint16_t tmp = (t) & 0x0FFFF; \
											vVCOM_LogChar(((tmp) >> 8) & 0x0FF);	\
											vVCOM_LogChar((tmp) & 0x0FF);			\
											vVCOM_LogChar((id)); }
#else
#define SCDL_ON_TASK_START(id,t)	 	{ }
#define SCDL_ON_TASK_STOP(id,t)	 		{ }
#endif

unsigned char tidCreateTask( void (*vTaskFunc)(void), unsigned long ulPeriod);
void vStartScheduler(void);
void vScdlTick1ms(void);

void vTaskSetState( taskID_t taskID, enum etypTaskStates eState);
void vSwitchAllTasksOff( void );
void vTaskSetPeriod( taskID_t taskID, unsigned long ulPeriod);
void vTaskInvokeDelayed( taskID_t taskID, unsigned long ulDelay);

unsigned char bSemaTake(sema_t* sema);
unsigned char bSemaCntTake(sema_t* sema);


#endif /*SCHEDULER_H_*/
