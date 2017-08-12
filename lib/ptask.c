/*
 * Author: Alessandro Trifoglio
 * Last revision: 22/12/2016
 */

/* H library */

#include "ptask.h"

/* VxWorks private libraries */

#include "tickLib.h" 								/*Clock tick library (for tickGet();)*/

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- Definitions --------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Events */

#define ACTIVATION 									0x9eb5a24c

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------------------------- Service routines ------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

void activate (const TASK_ID id) {

	/*if (taskIdVerify (id) == ERROR) return;*/

	if (eventSend (id, ACTIVATION) == ERROR) {
		;
	}

}

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------- Main functions -------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */


/* Catch the first activation time and set both the absolute deadline for the initial cycle and the next activation time; then, create a watchdog */

void wait_for_activation (task_attr_t * const attr) {

	const ULONG now = tickGet(); 															/*Catch the absolute activation time (system ticks)*/
	const WDOG_ID wid = wdCreate(); 														/*Create a watchdog timer to add activation alarms*/
	
	attr->wid = wid;
	attr->activation = now;
	attr->starting = now;
	attr->startingDelay = 0;
	
	/*printf ("First activation for %d with period %u ms at: %lu\n", attr->t, attr->period, now);*/

	attr->ad = now + (sysClkRateGet()*attr->deadline)/1000; 								/*Update the absolute deadline*/
	attr->na = now + (sysClkRateGet()*attr->period)/1000; 									/*Update the next activation time*/
	
}

/* At the end of each cycle, check if the deadline has been missed, and if so evaluate how many deadlines have been missed */

boolean deadline_miss (task_attr_t * const attr) {

	const ULONG now = tickGet();

	unsigned int i;
	for (i = 0; now > ((attr->ad) + i*(attr->period)); i++) {
		attr->misses++;
	}
		
	if (i > 0) return true;

	return false;

}

/* At the end of the current cycle, set an alarm to be fired at the next activation time and then wait until the alarm expires */

STATUS wait_for_period (task_attr_t * const attr) {

	STATUS ret = OK;
	ULONG na = attr->na; 																	/*Next activation time*/

	taskLock(); 																			/*No preemption to grant consistency among 'now' and wd activation*/

	ULONG now = tickGet(); 																	/*Catch the end of current cycle*/
	
	if (now >= na) { 																		/*If 'na' has been overrun, a new 'na' must be evaluated*/
		ULONG period = (sysClkRateGet()*attr->period)/1000;
		na += (1 + (now - na) / period) * period;
	}

	/*printf ("End of cycle for %d at: %lu. Next activation at: %lu. Waiting...\n", attr->t, now, na);*/

	wdStart (attr->wid, na - now, (FUNCPTR)activate, taskIdSelf()); 						/*Start the wd timer with the appropriate delay*/
	
	taskUnlock(); 																			/*Release preemption lock*/
	
	if (eventReceive (ACTIVATION, EVENTS_WAIT_ANY, WAIT_FOREVER, NULL) == ERROR) { 			/*Sleep until the wd timer has delivered the activation event*/
		ret = ERROR;
	}

	attr->finishing = now; 																	/*Finishing time*/
	attr->et = now - attr->starting; 														/*Elaboration time*/
	unsigned int et = (1000000*attr->et)/sysClkRateGet(); 									/*Elaboration time in microseconds (us)*/
	if (et > attr->wcet) attr->wcet = et; 													/*If this elaboration has been longer than 'wcet', update it*/

	attr->ad = na + (sysClkRateGet()*attr->deadline)/1000; 									/*Update the absolute deadline*/
	attr->na = na + (sysClkRateGet()*attr->period)/1000; 									/*Update the next activation time*/
	
	now = tickGet(); 																		/*Start of the new cycle*/
	attr->starting = now;
	attr->startingDelay = now - na;
	
	/*printf ("Activation for %d at: %lu\n", attr->t, now);*/

	return ret;

}
