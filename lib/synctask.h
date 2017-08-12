/*
 * This library allows the creation and the cancellation of tasks from a VxWorks process; furthermore, it provides some synchronization facilities among
 * tasks that are not natively present in this OS, such as the join(); function (that is instead provided by pthread.h library). These functions exclusively
 * use VxWorks services and not also the POSIX ones
 * Author: Alessandro Trifoglio
 * Last revision: 22/12/2016
 */

#ifndef SYNCTASK_H
#define SYNCTASK_H

/* Project root library */

#include "root.h"

/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- Definitions ---------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Messages (STATUS) */

#define WAITING 									0x7338a542
#define TASK_CANCELLED 								0x069ee30f
#define MAX_SPAWNEDTASKS_REACHED 					0x5c3f3ddb
#define MAX_LISTENINGTASKS_REACHED 					0xfbcd2cdb 
#define SYNC_FAULT 									0x6b6c4351

/* Events */

#define GENERIC 									0xb2813393

/* Flags */

#define SYNC_INVERSION_SAFE 						0x00000001

/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ------------- Task management routines: syntax is POSIX-like but these functions actually encapsulate VxWorks services and not pthread ones ------------- */
/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Initialize stcv and create mutex */

void initSync (void);

/* Return a pointer to the attribute structure of a task with identifier 't' */

task_attr_t* task_attr (const task_t t);

/* Return the identifier of the task with the specified name, if it exists */

task_t task_get (const char * const name);

/* Return the identifier of the calling task */

task_t task_self (void);

/* Create a VxWorks task and add it to stcv (spawned tasks control vector). NB: don't use the forward-slash character "/" in the 'name' field */

STATUS task_create_ (char * const name, task_attr_t * const attr, FUNCPTR body, const int * const arg);

/* Overload of the previous routine for 'body' with a single integer 'arg' */

STATUS task_create (char * const name, task_attr_t * const attr, FUNCPTR body, const int arg);

/* Delay the calling task for the specified number of microseconds 'us' */

STATUS task_delay (unsigned int us);

/* Suspend the calling task */

STATUS task_suspend (void);

/* Resume the task with the identifier 't' */

STATUS task_resume (const task_t t);

/* Wait for a specified task 't' to signal specific 'events' (at least one of them: EVENTS_WAIT_ANY semantics): this routine generalizes the join(); */

STATUS task_wait (const task_t t, const unsigned int events, const unsigned int flags);

/* Signal to all waiting tasks that specific 'events' have occurred */

STATUS task_signal (const unsigned int events, const unsigned int flags);

/* Wait for a specified task 't' to be cancelled */

STATUS task_join (const task_t t);

/* Signal to all listening tasks that the specified task 't' has been cancelled, remove it from stcv and delete it from the system */

STATUS task_cancel (const task_t t);

/* Cancel the calling task. WARNING: in order for this library to properly work, this routine MUST BE INTRODUCED AT THE END of all tasks' body */

STATUS task_exit (void);

#endif
