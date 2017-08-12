/* 
 * This library contains functions to use inside the body of a periodic task; their purpose is to manage the lifecycle of task evaluating activation times,
 * setting alarms to awake them at each period and so on
 * Author: Alessandro Trifoglio
 * Last revision: 20/12/2016
 */

#ifndef PTASK_H
#define PTASK_H

/* Project root library */

#include "root.h"

/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ------------- Time management routines: syntax is POSIX-like but these functions actually encapsulate VxWorks services and not pthread ones ------------- */
/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Catch the first activation time and set both the absolute deadline for the initial cycle and the next activation time; then, create a watchdog */

void wait_for_activation (task_attr_t * const attr);

/* At the end of each cycle, check if the deadline has been missed, and if so evaluate how many deadlines have been missed */

boolean deadline_miss (task_attr_t * const attr);

/* At the end of the current cycle, set an alarm to be fired at the next activation time and then wait until the alarm expires */

STATUS wait_for_period (task_attr_t * const attr);

#endif
