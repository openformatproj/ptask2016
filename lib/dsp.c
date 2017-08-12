/*
 * Author: Alessandro Trifoglio
 * Last revision: 21/12/2016
 */

/* H library */

#include "dsp.h"

/* Generic private libraries */

#include "math.h"

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- Definitions --------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Pi */

#define PI 											3.14159

/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------- Main functions -------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Evaluate the Fourier Transform of 'n' samples contained in 'frameT' using the algebraic definition, and write them in 'frameF' */

void sft (const double * const frameT, complex * const frameF, const unsigned int n) {

	complex sum;

	unsigned int p, q;

	for (p = 0; p < n; p++) {
		sum.real = 0;
		sum.imag = 0;
		for (q = 0; q < n; q++) {
			sum.real += frameT[q]*cos((2*PI*p*q)/n);
			sum.imag -= frameT[q]*sin((2*PI*p*q)/n);
		}
		frameF[p] = sum;
	}

}
