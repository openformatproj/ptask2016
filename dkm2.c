/*
 * Test 2. Please see documentation for a complete description.
 * Author: Alessandro Trifoglio
 * Last revision: 22/12/2016
 */

/* VxWorks libraries */

#include "ioLib.h" 									/*I/O interface library*/
#include "sockLib.h" 								/*Generic socket library*/
#include "sysLib.h" 								/*System-dependent library (for sysClkRateSet();)*/

/* Project libraries */

#include "lib/ptask.h"
#include "lib/synctask.h"
#include "lib/dsp.h"
#include "lib/dspIO.h"

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- Definitions --------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Number of created tasks: don't exceed 155 (0<=NT<=155) */

#define NT 											3

/* Base priority for VxWorks user tasks */

#define MAX_USER_PRIO								101

/* Whether to use priority inheritance (SYNC_INVERSION_SAFE) or not (~SYNC_INVERSION_SAFE) */

#define FLAGS										SYNC_INVERSION_SAFE

/* Length of a frame for FFT computation (use a power of 2). Computation length of 'task1' is affected by this parameter */

#define FRAME_LENGTH								256

/* Server IP Address */

#define SERVER_IP_ADDRESS 							"127.0.0.1"

/* Server Port Number */

#define SERVER_PORT_NUM 							5002

/* Input and output files */

#define INPUT_FILE 									"wave.txt"
#define OUTPUT_FILE 								"spectrum.txt"

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* -------------------------------------------------------- Internal data structures and variables -------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

int input; 																					/*Handle of device waveform comes from*/
int output; 																				/*Handle of device where spectrum has to be sent*/
int UDPSocket; 																				/*Handle of UDP socket*/

boolean inputAvailable; 																	/*Used to broadcast when input is not available anymore*/

double frameT[FRAME_LENGTH]; 																/*Frame where time samples are put before being processed by FFT*/
double frameT_[FRAME_LENGTH]; 																/*Buffer used by 'task1'*/
complex frameF[FRAME_LENGTH]; 																/*Frame where frequency samples are put before being sent*/

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------------------------- Service routines ------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Exit activity for generic periodic task */

void exitActivity (const int i) {

	switch (i) {
		case (0):
			close (input);
			break;
		case (1):
			close (output);
			break;
		case (2):
			close (UDPSocket);
			break;
		default:
			break;
	}
	
	printf ("Cancellation of task%u.\n", i);

	task_exit(); 																			/*Mandatory: see synctask.h for more details*/

}

/* Deadline miss management activity */

void deadlineMissActivity (const int i, const task_attr_t * const attr) {

	printf ("task%u has missed one or more deadlines between %lu and %lu. Total: %u.\n", i, attr->starting, attr->na, attr->misses);

}

/* Periodic activity for generic periodic task */

void periodicActivity (const int i) {

	switch (i) {
		case (0):
			if (acquireFromFile (input, frameT, FRAME_LENGTH) == EOF_REACHED) { 			/*Put waveform samples in memory ('frameT')*/
				inputAvailable = false;
				exitActivity (0);
			}
			task_wait (task_get ("task2"), GENERIC, FLAGS); 								/*Wait for 'task2' sending data in 'frameT'*/
			break;
		case (1):
			if (!inputAvailable) exitActivity (1);
			taskLock();
			frmcpy (frameT, frameT_, FRAME_LENGTH); 										/*Atomically copy 'frameT' into 'frameT_' to grant consistency*/								
			taskUnlock();
			sft (frameT_, frameF, FRAME_LENGTH); 											/*Evaluate the sft of 'frameT' and put results into 'frameF'*/
			sendToFile (output, frameF, FRAME_LENGTH); 										/*Send results to the output device*/
			break;
		case (2):
			if (!inputAvailable) exitActivity (2);
			if (sendToUDP (UDPSocket, frameT, FRAME_LENGTH) == ERROR) { 					/*Send waveform samples to UDP socket*/
				perror ("UDP SENDING FAILED");
			}										
			task_signal (GENERIC, FLAGS); 													/*Wake 'task0' after sending data*/
			break;
		default:
			break;
	}

}

/* Initial activity for generic periodic task */

void initActivity (const int i) {

	switch (i) {
		case (0):
			input = open (INPUT_FILE, O_RDONLY, 0444); 										/*Open the device waveform comes from*/
			break;
		case (1):
			output = open (OUTPUT_FILE, O_WRONLY, 0644); 									/*Open the device where spectrum has to be sent*/
			break;
		case (2):
			if ((UDPSocket = socket (AF_INET, SOCK_DGRAM, 0)) == ERROR) { 					/*Create and UDP socket*/
				perror ("SOCKET CREATION FAILED");
			}
			if (initUDP (UDPSocket, SERVER_IP_ADDRESS, SERVER_PORT_NUM) == ERROR) { 		/*Initialize UDP connection*/
				perror ("UNKNOWN SERVER NAME");
			}
			break;
		default:
			break;
	}

}

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------- Main functions -------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Periodic task body */

void body (int i) {

	task_attr_t *attr = task_attr (task_self()); 											/*Attribute structure of the calling task*/
	
	wait_for_activation (attr);

	initActivity (i);
	
	while (true) {
		periodicActivity (i);
		if (deadline_miss (attr)) deadlineMissActivity (i, attr);
		wait_for_period (attr);
	}

	exitActivity (i);

}

/* Init VxWorks function */

void init () {
	
	/*sysClkRateSet(1000);*/
	
	inputAvailable = true;

	task_attr_t attr[NT]; 																	/*Tasks' attributes list*/
	
	initSync(); 																			/*Init synctask.h data: put this before any other related routine*/
	
	unsigned int period;
	char name[15];
	STATUS st;
	unsigned int i;
	for (i = 0; i < NT; i++) {
		switch (i) {
			case (0):
				period = 40; 																/*'task0' period*/
				break;
			case (1):
				period = 80; 																/*'task1' period*/
				break;
			case (2):
				period = 40; 																/*'task2' period*/
				break;
			default:
				break;
		}
		initAttr (&attr[i], 1024, MAX_USER_PRIO + i, period, period); 						/*Populate tasks' attributes: relative deadline equal to period*/
		sprintf (name, "task%u", i);
		st = task_create (name, &attr[i], (FUNCPTR)body, (int)i); 							/*Create the task*/
		printf ("Creation of task%u. Status: 0x%08x\n", i, (unsigned int)st);
	}
	
	task_suspend();

}
