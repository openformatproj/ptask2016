/*
 * Test 1. Please see documentation for a complete description.
 * Author: Alessandro Trifoglio
 * Last revision: 22/12/2016
 */

/* Generic libraries */

#include "math.h"

/* VxWorks libraries */

#include "tickLib.h" 								/*Clock tick library (for doNothing(); function)*/

/* Project libraries */

#include "lib/ptask.h"
#include "lib/synctask.h"

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- Definitions --------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Number of created tasks: don't exceed 155 (0<=NT<=155) */

#define NT 											3

/* Base priority for VxWorks user tasks */

#define MAX_USER_PRIO								101

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------------------------- Service routines ------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Occupy CPU for 'ms' milliseconds */

void doNothing (const unsigned int ms) {

	ULONG now = tickGet();
	ULONG tick = 0;
	
	ULONG now_;
	
	while (true) {
		do {
			now_ = tickGet();
		}
		while (now_ == now);
		tick++;
		if (tick >= (sysClkRateGet()*ms)/1000) return;
		now = now_;
	}

}

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------- Main functions -------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Periodic task body */

void body (int i) {

	task_attr_t *attr = task_attr (task_self()); 											/*Attribute structure of the calling task*/

	wait_for_activation (attr);
	
	while (true) {
		doNothing (300); 																	/*Occupy the CPU for 300 milliseconds*/
		wait_for_period (attr);
	}

	task_exit(); 																			/*Mandatory: see synctask.h for more details*/

}

/* Init VxWorks function */

void init () {

	task_attr_t attr[NT]; 																	/*Tasks' attributes list*/
	
	initSync(); 																			/*Init synctask.h data: put this before any other related routine*/
	
	unsigned int period;
	char name[15];
	STATUS st;
	unsigned int i;
	for (i = 0; i < NT; i++) {
		period = 1000 * pow (2, i); 														/*Increasing periods: i=0 -> 1 sec, i=1 -> 2 sec, i=2 -> 4 sec...*/
		initAttr (&attr[i], 1024, MAX_USER_PRIO + i, period, period); 						/*Populate tasks' attributes: relative deadline equal to period*/
		sprintf (name, "task%u", i);
		st = task_create (name, &attr[i], (FUNCPTR)body, (int)i); 							/*Create the task*/
		printf ("Creation of task%u. Status: 0x%08x\n", i, (unsigned int)st);
	}
	
	task_suspend();

}
