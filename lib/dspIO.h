/*
 * This library provides functions to interface DSP computations towards extern devices
 * Author: Alessandro Trifoglio
 * Last revision: 22/12/2016
 */

#ifndef DSPIO_H
#define DSPIO_H

/* Parent library */

#include "dsp.h"

/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- Definitions ---------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Messages (STATUS) */

#define EOF_REACHED 								0x776a7db0

/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---------------------------------------------------------- IO functions used to access devices ---------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Acquire 'n' data from file 'input' and put them into 'frameT' */

STATUS acquireFromFile (const int input, double * const frameT, const unsigned int n);

/* Send 'n' complex data taken from 'frameF' to file 'output' */

void sendToFile (const int output, const complex * const frameF, const unsigned int n);

/* Init UDP connection structure. Credits to: Daniel Casini, VxWorks UDP Communication Demo developed in ReTiS Lab, 28/11/2016 */

STATUS initUDP (const int UDPSocket, char * const ip, const unsigned int port);

/* Send 'n' double data taken from 'frameT' to 'UDPSocket'. Credits to: Daniel Casini, VxWorks UDP Communication Demo developed in ReTiS Lab, 28/11/2016 */

STATUS sendToUDP (const int UDPSocket, const double * const frameT, const unsigned int n);

/* Copy a frame 'frameIn' into another 'frameOut' with the same length 'n' */

void frmcpy (const double * const frameIn, double * const frameOut, const unsigned int n);

#endif
