#ifndef EQUATION_OF_STATE_H_INCLUDED
#define EQUATION_OF_STATE_H_INCLUDED

#include "utils.h"
#include "parametres.h"

#ifdef HMPP

#endif

void equation_of_state (int imin,
			int imax,
			const int Hnxyt,
			const int Hnvar,
			const real Hsmallc,
			const real Hgamma,
			const int slices, const int Hstep,
			real *eint, real *q, real *c);

#endif // EQUATION_OF_STATE_H_INCLUDED
