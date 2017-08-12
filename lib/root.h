/* 
 * Project root library. This file only contains the definition of a structure whose purpose is to collect all the properties related to tasks, and a function
 * that can be used to fill them
 * Author: Alessandro Trifoglio
 * Last revision: 22/12/2016
 */

#ifndef ROOT_H
#define ROOT_H

/* Generic common libraries (for debug) */

#include "stdio.h"

/* VxWorks common libraries */

#include "vxworks.h"

#include "taskLib.h" 								/*Task management library*/
#include "sysLib.h" 								/*System-dependent library (for time to ticks conversion through sysClkRateGet();)*/
#include "wdLib.h" 									/*Watchdog timer library*/

/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- Definitions ---------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Types */

typedef int task_t; 								/*Identifier for tasks*/

/* Shared constants */

#define MAX_SPAWNEDTASKS 							256
#define MAX_NAME_LENGTH 							30

/* Flags */

#define DEBUG 										0xF0000000

/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------------ Shared (root) data structures and variables ------------------------------------------------------ */
/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Boolean type */

typedef enum boolean {
	false = 0,
	true = 1
} boolean;

/* This structure contains both static attributes that must be defined when a periodic task is created and dynamic values used for run-time management */

typedef struct task_attr_t {

	task_t t; 										/*Internal identifier comprised between 0 and MAX_SPAWNEDTASKS-1*/
	char name[MAX_NAME_LENGTH]; 					/*Name*/

	unsigned int stack; 							/*Stack size (bytes)*/
	
	unsigned int priority; 							/*Priority value in the user range [101, 255]*/
	unsigned int period; 							/*Period of execution in milliseconds (ms)*/
	unsigned int deadline; 							/*Relative deadline in milliseconds (ms): it must be less than the period*/
	
	unsigned long wcet; 							/*Worst case computation time in microseconds (us)*/

	/* Dynamic parameters */

	unsigned int dynamicPrio; 						/*Dynamic priority (for priority inversion avoidance)*/

	unsigned int misses; 							/*Number of deadline misses*/

	WDOG_ID wid; 									/*VxWorks watchdog identifier (used to trigger periodic activations)*/

	ULONG activation; 								/*First activation time (system ticks)*/

	ULONG starting; 								/*Last starting time of computation (system ticks)*/
	ULONG startingDelay; 							/*Last starting delay (difference among starting time and activation time, in system ticks)*/

	ULONG finishing; 								/*Last time at which computation has finished (system ticks)*/
	ULONG et; 										/*Last elaboration time (system ticks)*/
	
	ULONG ad; 										/*Absolute deadline (system ticks)*/
	ULONG na; 										/*Next activation time (system ticks)*/

} task_attr_t;

/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ----------------------------------------------------------- Attribute initialization function ----------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Populate attribute structure 'attr' with static information */

void initAttr (task_attr_t * const attr, const unsigned int stack, const unsigned int priority, const unsigned int period, const unsigned int deadline);

#endif
