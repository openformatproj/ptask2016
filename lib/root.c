/*
 * Author: Alessandro Trifoglio
 * Last revision: 20/12/2016
 */

/* H library */

#include "root.h"

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------- Main functions -------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Populate attribute structure 'attr' with static information */

void initAttr (task_attr_t * const attr, const unsigned int stack, const unsigned int priority, const unsigned int period, const unsigned int deadline) {

	attr->stack = stack;
	attr->priority = priority;
	attr->period = period;
	attr->deadline = deadline;

	attr->wcet = 0; 																		/*If not specified, put 'wcet' to zero*/

}