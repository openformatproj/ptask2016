/*
 * Author: Alessandro Trifoglio
 * Last revision: 22/12/2016
 */

/* H library */

#include "dspIO.h"

/* Generic private libraries */

#include "stdlib.h" 								/*For atof(); utility*/
/*#include "string.h" 								For strlen(); utility*/

/* VxWorks private libraries */

#include "ioLib.h" 									/*I/O interface library*/
#include "sockLib.h" 								/*Generic socket library*/
#include "inetLib.h" 								/*Internet address manipulation routines*/
#include "hostLib.h" 								/*Host table subroutine library*/

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- Definitions --------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Maximum number of characters for double values in input file: for instance, '-10.6887' has 8 characters */

#define MAX_CHARACTERS 								10

/* Maximum absolute value for doubles. Be consistent with the previous constraint, taking into account 4 digits of precision */

#define MAX_ABSOLUTE_VALUE 							1000

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* -------------------------------------------------------- Internal data structures and variables -------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Server socket address */

struct sockaddr_in serverAddr;

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------- Main functions -------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Acquire 'n' data from file 'input' and put them into 'frameT' */

STATUS acquireFromFile (const int input, double * const frameT, const unsigned int n) {

	char buffer[MAX_CHARACTERS+2]; 															/*It may contain for instance '-10.6887\n\0'*/
	
	boolean EOFFound = false;
	unsigned int index;

	boolean newLineFound;
	unsigned int i;
	
	for (index = 0; index < n; index++) {
		
		newLineFound = false;
		for (i = 0; !EOFFound && !newLineFound && i < MAX_CHARACTERS+2; i++) {
			if (read (input, buffer + i, 1) == 0) EOFFound = true;
			else if (buffer[i] == '\n') { 													/*Single values are separated by a newline character*/
				buffer[i] = '\0';
				newLineFound = true;
			}
		}
		
		if (EOFFound) return EOF_REACHED; 													/*If end of file has been reached, return*/

		frameT[index] = atof (buffer); 														/*Convert the content of 'buffer' into a double*/
	
	}
	
	return OK;

}

/* Send 'n' complex data taken from 'frameF' to file 'output' */

void sendToFile (const int output, const complex * const frameF, const unsigned int n) {

	char buffer[2*MAX_CHARACTERS+8]; 														/*It may contain for instance '-10.6887 + j(-10.6887)\n\0'*/
	int len;
	
	unsigned int index;

	for (index = 0; index < n; index++) {

		if (frameF[index].real < MAX_ABSOLUTE_VALUE && frameF[index].imag < MAX_ABSOLUTE_VALUE) {
			len = sprintf (buffer, "%+.4f + j(%+.4f)\n", frameF[index].real, frameF[index].imag);
		}
		else len = sprintf (buffer, "NaN\n"); 												/*IEEE arithmetic representation for Not a Number*/
		
		write (output, buffer, len);

	}

}

/* Init UDP connection structure. Credits to: Daniel Casini, VxWorks UDP Communication Demo developed in ReTiS Lab, 28/11/2016 */

STATUS initUDP (const int UDPSocket, char * const ip, const unsigned int port) {

	int sockAddrSize = sizeof (struct sockaddr_in); 										/*Size of socket address structure*/

	/*bzero ((char*)&serverAddr, sockAddrSize);*/
	serverAddr.sin_len = (u_char)sockAddrSize;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons (port);

	if ((serverAddr.sin_addr.s_addr = inet_addr (ip)) == ERROR &&
		(serverAddr.sin_addr.s_addr = hostGetByName (ip)) == ERROR) {
		close (UDPSocket);
		return ERROR;
	}

	return OK;

}

/* Send 'n' double data taken from 'frameT' to 'UDPSocket'. Credits to: Daniel Casini, VxWorks UDP Communication Demo developed in ReTiS Lab, 28/11/2016 */

STATUS sendToUDP (const int UDPSocket, const double * const frameT, const unsigned int n) {

	char buffer[MAX_CHARACTERS+2]; 															/*It may contain for instance '-10.6887\n\0'*/
	int len;

	unsigned int index;

	for (index = 0; index < n; index++) {

		len = sprintf (buffer, "%+.4f\n", frameT[index]);

		if ((len = sendto (UDPSocket, buffer, len, 0, (struct sockaddr*)&serverAddr, (int)serverAddr.sin_len)) == ERROR) {
			close (UDPSocket);
			return ERROR;
		}

	}

	return OK;

}

/* Copy a frame 'frameIn' into another 'frameOut' with the same length 'n' */

void frmcpy (const double * const frameIn, double * const frameOut, const unsigned int n) {
	
	unsigned int index;
	
	for (index = 0; index < n; index++) {
		
		frameOut[index] = frameIn[index];
		
	}
	
}
