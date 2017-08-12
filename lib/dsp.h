/*
 * This library provides functionalities related to digital signal processing (DSP)
 * Author: Alessandro Trifoglio
 * Last revision: 21/12/2016
 */

#ifndef DIGITALPROCESS_H
#define DIGITALPROCESS_H

/* Project root library */

#include "root.h"

/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------------ Shared (root) data structures and variables ------------------------------------------------------ */
/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Complex type */

typedef struct complex {

	double real;
	double imag;

} complex;

/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- DSP -------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Evaluate the Fourier Transform of 'n' samples contained in 'frameT' using the algebraic definition, and write them in 'frameF' */

void sft (const double * const frameT, complex * const frameF, const unsigned int n);

#endif