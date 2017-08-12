/*
 * Author: Alessandro Trifoglio
 * Last revision: 22/12/2016
 */

/* H library */

#include "synctask.h"

/* Generic private libraries */

#include "string.h"

/* VxWorks private libraries */

#include "semLib.h" 								/*ME (mutual exclusion) semaphore library*/

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- Definitions --------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Max number of listening tasks (for each spawned task) */

#define MAX_LISTENINGTASKS 							20

/* Messages (STATUS) */

#define SPAWNEDTASK_PRESENT 						0xf49d93f9
#define SPAWNEDTASK_ABSENT 							0x96033890
#define LISTENINGTASK_PRESENT 						0xf6fdc830
#define LISTENINGTASK_ABSENT 						0xfa31e94c

/* Events */

#define CANCELLED 									0x3487dfa4 

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* -------------------------------------------------------- Internal data structures and variables -------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Listening tasks control block */

struct listeningTaskCB {

	boolean valid; 									/*If the block contains a valid (active) task*/
	
	task_t t; 										/*Identifier of the listening task*/

	unsigned int priority; 							/*Priority of the listening task when it's inserted in the list*/
	
	unsigned int events; 							/*The events the associated task is listening for*/
	
	struct listeningTaskCB *nextArrived; 			/*Reference to the next valid control block of the listening tasks control vector (FIFO)*/
	struct listeningTaskCB *prevArrived; 			/*Reference to the previous valid control block of the listening tasks control vector (FIFO)*/
	struct listeningTaskCB *nextPrio; 				/*Reference to the next valid control block of 'ltcv' with higher priority (PRIO)*/
	struct listeningTaskCB *prevPrio; 				/*Reference to the previous valid control block of 'ltcv' with lower priority (PRIO)*/

};

typedef struct listeningTaskCB listeningTaskCB; 	/*Workaround to correctly define a linked-list block structure type in C*/

/* Listening tasks entry block */

typedef struct listeningTaskEB {

	unsigned int listeningTasks; 					/*Number of listening tasks*/
	
	int index_ltcv;									/*Index of the current first free position in 'ltcv' (-1 if no positions available)*/
	listeningTaskCB *firstArrived; 					/*Reference to the first valid control block of the listening tasks control vector (FIFO)*/
	listeningTaskCB *lastArrived; 					/*Reference to the last valid control block of the listening tasks control vector (FIFO)*/
	listeningTaskCB *firstPrio; 					/*Reference to the most privileged (lowest priority) valid control block of 'ltcv' (PRIO)*/
	listeningTaskCB *lastPrio; 						/*Reference to the least privileged (highest priority) valid control block of 'ltcv' (PRIO)*/

} listeningTaskEB;

/* Spawned tasks control block */

typedef struct spawnedTaskCB {

	boolean valid; 									/*If the block contains a valid (active) task*/

	task_attr_t *attr; 								/*Pointer to a task attribute structure*/
	
	TASK_ID id; 									/*VxWorks task identifier*/
	
	boolean waiting; 								/*If the task is currently waiting for events*/
	
	/*WIND_TCB *wtcb;*/ 							/*Reference to the task's VxWorks control block*/
	/*TASK_DESC *descr;*/
	
	listeningTaskEB lteb; 							/*Entry block of the listening tasks control vector (see below)*/
	listeningTaskCB ltcv[MAX_LISTENINGTASKS]; 		/*Listening tasks control vector: it lists all tasks listening for events associated with owning task*/

} spawnedTaskCB;

/* Number of spawned (created) tasks */

unsigned int spawnedTasks;

/* Index of the current first free position in stcv (-1 if no positions available) */

int index_stcv;

/* General spawned tasks control vector */

spawnedTaskCB stcv[MAX_SPAWNEDTASKS];

/* VxWorks mutex */

SEM_ID mutex;

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------------------- Vectors management functions ------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Check if a task 't' is present in the listening tasks control vector 'ltcv': it returns the associated index if the task is found, -1 otherwise */

int checklTask (const task_t t, const listeningTaskCB * const ltcv) {

	int index = -1;
	const listeningTaskCB * ltcb;

	unsigned int index_;
	for (index_ = 0; index == -1 && index_ < MAX_LISTENINGTASKS; index_++) { 				/*Scan 'ltcv' to find the requested task*/
		ltcb = &ltcv[index_];
		if (ltcb->valid && (ltcb->t == t)) index = index_;
	}

	return index;

}

/* Add a task 't' to the ltcv of a spawned task 't_', listening to incoming 'events' */

STATUS addlTask (const task_t t, const task_t t_, const unsigned int events) {

	spawnedTaskCB * const stcb = &stcv[(unsigned int)t_];
	listeningTaskEB * const lteb = &stcb->lteb;
	listeningTaskCB * const ltcv = stcb->ltcv;
	listeningTaskCB * const ltcb = &ltcv[(unsigned int)lteb->index_ltcv];
	const unsigned int priority = stcv[(unsigned int)t].attr->dynamicPrio; 					/*Priority of the listening task to be inserted*/

	if (checklTask (t, ltcv) != -1) return LISTENINGTASK_PRESENT; 							/*If the task is already present, don't add it and return*/
	if (lteb->index_ltcv == -1) return MAX_LISTENINGTASKS_REACHED; 							/*'ltcv' is full*/

	ltcb->valid = true;
	ltcb->t = t;
	ltcb->priority = priority; 																/*Set the 'ltcb' priority to the current priority of 't'*/
	ltcb->events = events;

	if (lteb->listeningTasks == 0) { 														/*If there aren't listening tasks yet...*/
		ltcb->nextArrived = NULL;
		ltcb->prevArrived = NULL;
		ltcb->nextPrio = NULL;
		ltcb->prevPrio = NULL; 																/*...set the new task's 'ltcb' pointer fields to NULL...*/
		lteb->firstArrived = ltcb;
		lteb->lastArrived = ltcb;
		lteb->firstPrio = ltcb;
		lteb->lastPrio = ltcb; 																/*...and set 'lteb' refs to point the incoming task's 'ltcb'...*/
	}
	else { 																					/*...otherwise, the last valid task is in 'lteb.last###'*/
		ltcb->nextArrived = NULL; 															/*FIFO management*/
		ltcb->prevArrived = lteb->lastArrived;
		lteb->lastArrived->nextArrived = ltcb;
		lteb->lastArrived = ltcb;
		if (priority < lteb->firstPrio->priority) { 										/*PRIO management. If the new task's priority is the lowest...*/
			ltcb->nextPrio = lteb->firstPrio; 												/*...put it on top...*/
			ltcb->prevPrio = NULL;
			lteb->firstPrio->prevPrio = ltcb;
			lteb->firstPrio = ltcb;
		}
		else if (priority >= lteb->lastPrio->priority) { 									/*...on the contrary, if it's the highest...*/
			ltcb->prevPrio = lteb->lastPrio; 												/*...put it on bottom...*/
			ltcb->nextPrio = NULL;
			lteb->lastPrio->nextPrio = ltcb;
			lteb->lastPrio = ltcb;
		}
		else { 																				/*...otherwise, scan the 'ltcv' until the right slot is found*/
			listeningTaskCB *toPreceed = lteb->firstPrio->nextPrio; 						/*Starting from the second block in the list...*/
			boolean positioned = false;
			unsigned int i;
			for (i = 1; !positioned && i < lteb->listeningTasks; i++) {
				if (priority < toPreceed->priority) { 										/*...see if the new task's priority is less than 'toPreceed'...*/
					ltcb->nextPrio = toPreceed; 											/*(in this case, put the new task between it and the previous)*/
					ltcb->prevPrio = toPreceed->prevPrio;
					toPreceed->prevPrio = ltcb;
					positioned = true;
				}
				else toPreceed = toPreceed->nextPrio; 										/*...otherwise, continue to iterate*/
			}
			if (!positioned) return SYNC_FAULT; 											/*Fault: listening task not positioned*/
		}
	}

	lteb->listeningTasks++;

	boolean indexFound = false;
	unsigned int index_;
	for (index_ = 0; !indexFound && index_ < MAX_LISTENINGTASKS; index_++) { 				/*Scan the 'ltcv' to find the first free index*/
		if (!ltcv[index_].valid) {
			lteb->index_ltcv = index_; 														/*When it's found, assign it to the current first free position*/
			indexFound = true;
		}
	}

	if (!indexFound) lteb->index_ltcv = -1; 												/*No free index found ('ltcv' is full)*/

	return OK;

}

/* Remove a task 't' from the ltcv of a spawned task 't_' */

STATUS removelTask (const task_t t, const task_t t_) {

	spawnedTaskCB * const stcb = &stcv[(unsigned int)t_];
	listeningTaskEB * const lteb = &stcb->lteb;
	listeningTaskCB * const ltcv = stcb->ltcv;

	const int index = checklTask (t, ltcv);
	if (index == -1) return LISTENINGTASK_ABSENT; 											/*If task 't' is not present, return now*/

	listeningTaskCB * const ltcb = &ltcv[(unsigned int)index];

	ltcb->valid = false;
	
	if (ltcb->nextArrived != NULL) ltcb->nextArrived->prevArrived = ltcb->prevArrived; 		/*Update the linked list (FIFO)*/
	else lteb->lastArrived = ltcb->prevArrived;
	if (ltcb->prevArrived != NULL) ltcb->prevArrived->nextArrived = ltcb->nextArrived;
	else lteb->firstArrived = ltcb->nextArrived;
	if (ltcb->nextPrio != NULL) ltcb->nextPrio->prevPrio = ltcb->prevPrio; 					/*Update the linked list (PRIO)*/
	else lteb->lastPrio = ltcb->prevPrio;
	if (ltcb->prevPrio != NULL) ltcb->prevPrio->nextPrio = ltcb->nextPrio;
	else lteb->firstPrio = ltcb->nextPrio;

	lteb->listeningTasks--;

	if (lteb->index_ltcv == -1) lteb->index_ltcv = index; 									/*If 'ltcv' was full, set 't' as new first free position*/

	return OK;

}

/* Check if a task with VxWorks identifier 'id' is already present in the stcv */

boolean checksTask (const TASK_ID id) {

	const spawnedTaskCB *stcb;
	unsigned int index_;
	for (index_ = 0; index_ < MAX_SPAWNEDTASKS; index_++) { 								/*Scan the 'stcv' to find the requested task*/
		stcb = &stcv[index_];
		if (stcb->valid && (stcb->id == id)) return true;
	}

	return false;

}

/* Add a task with VxWorks identifier 'id' and attributes in 'attr' to the stcv, and initialize its stcb */

STATUS addsTask (const TASK_ID id, task_attr_t * const attr, const char * const name) {

	if (checksTask (id)) return SPAWNEDTASK_PRESENT; 										/*If the task is already present, don't add it and return*/
	if (index_stcv == -1) return MAX_SPAWNEDTASKS_REACHED;

	spawnedTaskCB * const stcb = &stcv[index_stcv];
	listeningTaskEB * const lteb = &stcb->lteb;
	listeningTaskCB * const ltcv = stcb->ltcv;
	
	stcb->valid = true;
	stcb->attr = attr;
	stcb->id = id;
	stcb->waiting = false;

	/*stcb.wtcb = taskTcb ((TASK_ID)t);*/
	/*taskGetInfo ((TASK_ID)t, stcb.descr);*/
	
	lteb->listeningTasks = 0;
	lteb->index_ltcv = 0;
	lteb->firstArrived = NULL;
	lteb->lastArrived = NULL;
	lteb->firstPrio = NULL;
	lteb->lastPrio = NULL;

	unsigned int index_;
	for (index_ = 0; index_ < MAX_LISTENINGTASKS; index_++) {
		ltcv[index_].valid = false; 														/*Initialize the associated 'ltcv'*/
	}

	attr->t = index_stcv; 																	/*Put the internal identifier in the task_attr_t structure...*/
	strcpy (attr->name, name); 																/*...and also the name...*/
	attr->dynamicPrio = attr->priority; 													/*...initialize the dynamic priority to the static one...*/
	attr->misses = 0; 																		/*...and initialize deadline misses to zero*/

	spawnedTasks++;

	if (spawnedTasks < MAX_SPAWNEDTASKS) {
		
		boolean indexFound = false;
		unsigned int index_;
		for (index_ = 0; !indexFound && index_ < MAX_SPAWNEDTASKS; index_++) { 				/*Scan the 'stcv' to find the first free index*/
			if (!stcv[index_].valid) {
				index_stcv = index_; 														/*When it's found, assign it to the current first free position*/
				indexFound = true;
			}
		}

	}
	else index_stcv = -1; 																	/*'stcv' is full*/

	return OK;

}

/* Remove a task 't' from stcv */

STATUS removesTask (const task_t t) {
	
	spawnedTaskCB * const stcb = &stcv[(unsigned int)t];

	if (!stcb->valid) return SPAWNEDTASK_ABSENT; 											/*If task 't' is not present, return now*/

	stcb->valid = false;
	
	wdDelete (stcb->attr->wid); 															/*Deallocate private watchdog timer*/

	spawnedTasks--;

	if (index_stcv == -1) index_stcv = (int)t; 												/*If 'stcv' was full, set 't' as new first free position*/

	return OK;

}

/* Convert a task's VxWorks 'id' into the corresponding task_t value: it returns -1 if 'id' is not found */

task_t taskT (const TASK_ID id) {
	
	const spawnedTaskCB *stcb;
	unsigned int index_;
	for (index_ = 0; index_ < MAX_SPAWNEDTASKS; index_++) {
		stcb = &stcv[index_];
		if (stcb->valid && (stcb->id == id)) return (task_t)index_;
	}

	return (task_t)-1; 																		/*The requested 'id' has not been found*/

}

/* Convert a task's identifier into the corresponding VxWorks value */

TASK_ID taskId (const task_t t) {

	return stcv[(unsigned int)t].id;

}

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------------------------- Service routines ------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Wait for 'events' associated to a specific task 't' (for instance, cancellation) */

STATUS waitFor (const task_t t, const unsigned int events, const unsigned int flags) {

	if (taskIdVerify (taskId (t)) == ERROR) return TASK_CANCELLED; 							/*Task has been cancelled or it doesn't exist at all*/

	const task_t tSelf = taskT (taskIdSelf());
	spawnedTaskCB * const stcbSelf = &stcv[(unsigned int)tSelf];

	/*unsigned int eventsReceived = 0;*/

	semTake (mutex, WAIT_FOREVER); 															/*Concurrent operations on 'ltcv' must be executed in ME*/
	const STATUS st = addlTask (tSelf, t, events); 											/*Add the caller to 'ltcv' of 't'*/
	semGive (mutex); 																		/*Leave ME*/

	if (st == SYNC_FAULT) {
		return SYNC_FAULT; 																	/*Fault: asking task can't be positioned in the 'ltcv' of 't'*/
	}

	if (st == LISTENINGTASK_PRESENT) {
		return SYNC_FAULT; 																	/*Fault: asking task is already in the 'ltcv' of 't'*/
	}
	
	if (st == MAX_LISTENINGTASKS_REACHED) {
		return MAX_LISTENINGTASKS_REACHED; 													/*'ltcv' is full*/
	}

	if ((flags & SYNC_INVERSION_SAFE) == SYNC_INVERSION_SAFE) {
		const spawnedTaskCB * const stcb = &stcv[(unsigned int)t]; 							/*'stcb' of task the listening tasks are waiting for*/
		const listeningTaskCB * const firstPrio = stcb->lteb.firstPrio; 					/*Most privileged task among all waiting for 't'*/
		if (stcb->attr->dynamicPrio > firstPrio->priority) {
			taskPrioritySet (taskId (t), firstPrio->priority); 								/*Set the priority of 't' to avoid priority inversion*/
			/*printf ("Priority of %d changed to %d\n", t, firstPrio->priority);*/
		}
	}

	stcbSelf->waiting = true;
	
	if (eventReceive (events, EVENTS_WAIT_ANY, WAIT_FOREVER, NULL) == ERROR) { 				/*Blocking here*/
		stcbSelf->waiting = false;
		return ERROR;
	}
	
	stcbSelf->waiting = false;
	
	return OK;

}

/* Signal all tasks waiting for 'events' associated to a specific task 't' (for instance, cancellation) */

STATUS signalThat (const task_t t, const unsigned int events, const unsigned int flags) {

	if (taskIdVerify (taskId (t)) == ERROR) return TASK_CANCELLED; 							/*Task has been cancelled or it doesn't exist at all*/

	const spawnedTaskCB * const stcb = &stcv[(unsigned int)t];
	const listeningTaskEB * const lteb = &stcb->lteb;

	semTake (mutex, WAIT_FOREVER); 															/*Concurrent operations on 'ltcv' must be executed in ME*/
	
	const listeningTaskCB *toWake = lteb->firstArrived; 									/*Start from the first listening task (put in 'toWake')*/
	unsigned int i;
	for (i = 0; i < lteb->listeningTasks; i++) {
		if (toWake->events & events) { 														/*If 'toWake' is listening for one of the occurred events...*/
			if (eventSend (taskId (toWake->t), events) == ERROR) { 							/*...send the occurrence to it...*/
				semGive (mutex); 															/*(leave ME before returning ERROR)*/
				return ERROR;
			}
			removelTask (toWake->t, t); 													/*...and remove it from the listening tasks' vector*/
		}
		toWake = toWake->nextArrived; 														/*Update toWake to the next listening task*/
	}

	if ((flags & SYNC_INVERSION_SAFE) == SYNC_INVERSION_SAFE) {
		const spawnedTaskCB * const stcb = &stcv[(unsigned int)t]; 							/*'stcb' of task the listening tasks are waiting for*/
		const listeningTaskCB * const firstPrio = stcb->lteb.firstPrio; 					/*Most privileged task among all tasks still in 'ltcv'*/
		if (firstPrio == NULL) {
			taskPrioritySet (taskId (t), stcb->attr->priority); 							/*If 'ltcv' is empty, restore the original priority...*/
			/*printf ("Priority of %d changed to %d\n", t, stcb->attr->priority);*/
		}
		else if (stcb->attr->dynamicPrio < firstPrio->priority) {
			taskPrioritySet (taskId (t), firstPrio->priority); 								/*...otherwise, set again the right priority*/
			/*printf ("Priority of %d changed to %d\n", t, firstPrio->priority);*/
		}
	}

	semGive (mutex); 																		/*Leave ME*/

	return OK;

}

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------- Main functions -------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Initialize stcv and create mutex */

void initSync (void) {
	
	unsigned int index_;
	for (index_ = 0; index_ < MAX_SPAWNEDTASKS; index_++) {
		stcv[index_].valid = false;
	}
	spawnedTasks = 0;
	index_stcv = 0;
	mutex = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);

}

/* Return a pointer to the attribute structure of a task with identifier 't' */

task_attr_t* task_attr (const task_t t) {

	return stcv[(unsigned int)t].attr;

}

/* Return the identifier of the task with the specified name, if it exists (otherwise, -1 is returned) */

task_t task_get (const char * const name) {

	const spawnedTaskCB *stcb;
	unsigned int index_;
	for (index_ = 0; index_ < MAX_SPAWNEDTASKS; index_++) {
		stcb = &stcv[index_];
		if (stcb->valid && (strcmp (stcb->attr->name, name) == 0)) return (task_t)index_;
	}

	return (task_t)-1; 

}

/* Return the identifier of the calling task */

task_t task_self (void) {

	return taskT (taskIdSelf());

}

/* Create a VxWorks task and add it to stcv (spawned tasks control vector). NB: don't use the forward-slash character "/" in the 'name' field */

STATUS task_create_ (char * const name, task_attr_t * const attr, FUNCPTR body, const int * const arg) {
	
	STATUS st;
	const TASK_ID id = taskSpawn (name, attr->priority, VX_FP_TASK, attr->stack, body, 
		arg[0], 
		arg[1], 
		arg[2], 
		arg[3], 
		arg[4], 
		arg[5], 
		arg[6], 
		arg[7], 
		arg[8], 
		arg[9]);
	
	semTake (mutex, WAIT_FOREVER); 															/*Concurrent operations on 'stcv' must be executed in ME*/
	st = addsTask (id, attr, name);
	semGive (mutex); 																		/*Leave ME*/
	
	if (st == SPAWNEDTASK_PRESENT) return SYNC_FAULT;
	if (st == MAX_SPAWNEDTASKS_REACHED) { 													/*Maximum number of tasks reached, delete the incoming one*/
		taskDelete (id);
		return MAX_SPAWNEDTASKS_REACHED;
	}

	return OK;

}

/* Overload of the previous routine for 'body' with a single integer 'arg' */

STATUS task_create (char * const name, task_attr_t * const attr, FUNCPTR body, const int arg) {

	STATUS st;
	const TASK_ID id = taskSpawn (name, attr->priority, VX_FP_TASK, attr->stack, body, 
		arg, 
		0, 
		0, 
		0, 
		0, 
		0, 
		0, 
		0, 
		0, 
		0);

	semTake (mutex, WAIT_FOREVER); 															/*Concurrent operations on 'ltcv' must be executed in ME*/
	st = addsTask (id, attr, name);
	semGive (mutex); 																		/*Leave ME*/
	
	if (st == SPAWNEDTASK_PRESENT) return SYNC_FAULT;
	if (st == MAX_SPAWNEDTASKS_REACHED) { 													/*Maximum number of tasks reached, delete the incoming one*/
		taskDelete (id);
		return MAX_SPAWNEDTASKS_REACHED;
	}

	return OK;

}

/* Delay the calling task for the specified number of microseconds 'us' */

STATUS task_delay (unsigned int us) {
	
	return taskDelay (sysClkRateGet()*us/1000000);

}

/* Suspend the calling task */

STATUS task_suspend (void) {

	return taskSuspend (taskIdSelf());

}

/* Resume the task with the identifier 't' */

STATUS task_resume (const task_t t) {

	return taskResume (taskId (t));

}

/* Wait for a specified task 't' to signal specific 'events' (at least one of them: EVENTS_WAIT_ANY semantics): this routine generalizes the join(); */

STATUS task_wait (const task_t t, const unsigned int events, const unsigned int flags) {

	return waitFor (t, events, flags); 														/*This is a blocking operation (VxWorks events are synchronous)*/

}

/* Signal to all waiting tasks that specific 'events' have occurred */

STATUS task_signal (const unsigned int events, const unsigned int flags) {

	return signalThat (task_self(), events, flags);

}

/* Wait for a specified task 't' to be cancelled */

STATUS task_join (const task_t t) {

	return waitFor (t, CANCELLED, ~SYNC_INVERSION_SAFE); 									/*This is a blocking operation (VxWorks events are synchronous)*/

}

/* Signal to all listening tasks that the specified task 't' has been cancelled, remove it from stcv and delete it from the system */

STATUS task_cancel (const task_t t) {

	STATUS st = removesTask (t);
	const TASK_ID id = taskId (t);

	if (st == SPAWNEDTASK_ABSENT) {
		if (taskIdVerify (id) == OK) { 														/*Fault: task active but not present in 'stcv'*/
			taskDelete (taskId (t));
			return SYNC_FAULT;
		}
		else return TASK_CANCELLED;
	}
	else {
		if (taskIdVerify (id) == ERROR) {
			return SYNC_FAULT; 																/*Fault: task not active but present in 'stcv'*/
		}
	}

	if (stcv[(unsigned int)t].waiting) return WAITING; 										/*Task is waiting for events and hence can't be cancelled*/

	signalThat (t, CANCELLED, ~SYNC_INVERSION_SAFE);
	
	return taskDelete (taskId (t));

}

/* Cancel the calling task. WARNING: in order for this library to properly work, this routine MUST BE INTRODUCED AT THE END of all tasks' body */

STATUS task_exit (void) {

	return task_cancel (task_self());

}
