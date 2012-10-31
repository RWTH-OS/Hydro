/*
  A simple 2D hydro code
  (C) Romain Teyssier : CEA/IRFU           -- original F90 code
  (C) Pierre-Francois Lavallee : IDRIS      -- original F90 code
  (C) Guillaume Colin de Verdiere : CEA/DAM -- for the C version
*/
/*

This software is governed by the CeCILL license under French law and
abiding by the rules of distribution of free software.  You can  use, 
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info". 

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability. 

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or 
data to be ensured and,  more generally, to use and operate it in the 
same conditions as regards security. 

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.

*/

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"
#include "hydro_utils.h"
#include "hydro_funcs.h"
void
hydro_init(hydroparam_t * H, hydrovar_t * Hv) {
  int i, j;
  int x, y;

  // *WARNING* : we will use 0 based arrays everywhere since it is C code!
  H->imin = H->jmin = 0;

  // We add two extra layers left/right/top/bottom
  H->imax = H->nx + ExtraLayerTot;
  H->jmax = H->ny + ExtraLayerTot;
  H->nxt = H->imax - H->imin;   // column size in the array
  H->nyt = H->jmax - H->jmin;   // row size in the array

  // maximum direction size
  H->nxyt = (H->nxt > H->nyt) ? H->nxt : H->nyt;
  // To make sure that slices are properly aligned, we make the array a
  // multiple of NDBLE double
#define NDBLE 16
  // printf("avant %d ", H->nxyt);
  // H->nxyt = (H->nxyt + NDBLE - 1) / NDBLE;
  // H->nxyt *= NDBLE;
  // printf("apres %d \n", H->nxyt);

  // allocate uold for each conservative variable
  Hv->uold = (double *) DMalloc(H->nvar * H->nxt * H->nyt);

  // wind tunnel with point explosion
  for (j = H->jmin + ExtraLayer; j < H->jmax - ExtraLayer; j++) {
    for (i = H->imin + ExtraLayer; i < H->imax - ExtraLayer; i++) {
      Hv->uold[IHvP(i, j, ID)] = one;
      Hv->uold[IHvP(i, j, IU)] = zero;
      Hv->uold[IHvP(i, j, IV)] = zero;
      Hv->uold[IHvP(i, j, IP)] = 1e-5;
    }
  }

  // Initial shock
  if (H->testCase == 0) {
    if (H->nproc == 1) {
      x = (H->imax - H->imin) / 2 + ExtraLayer * 0;
      y = (H->jmax - H->jmin) / 2 + ExtraLayer * 0;
      Hv->uold[IHvP(x, y, IP)] = one / H->dx / H->dx;
      printf("Centered test case : %d %d\n", x, y);
    } else {
      x = ((H->globnx) / 2);
      y = ((H->globny) / 2);
      if ((x >= H->box[XMIN_BOX]) && (x < H->box[XMAX_BOX]) && (y >= H->box[YMIN_BOX]) && (y < H->box[YMAX_BOX])) {
        x = x - H->box[XMIN_BOX] + ExtraLayer;
        y = y - H->box[YMIN_BOX] + ExtraLayer;
        Hv->uold[IHvP(x, y, IP)] = one / H->dx / H->dx;
        printf("Centered test case : [%d] %d %d\n", H->mype, x, y);
      }
    }
  }
  if (H->testCase == 1) {
    if (H->nproc == 1) {
      x = ExtraLayer;
      y = ExtraLayer;
      Hv->uold[IHvP(x, y, IP)] = one / H->dx / H->dx;
      printf("Lower corner test case : %d %d\n", x, y);
    } else {
      x = ExtraLayer;
      y = ExtraLayer;
      if ((x >= H->box[XMIN_BOX]) && (x < H->box[XMAX_BOX]) && (y >= H->box[YMIN_BOX]) && (y < H->box[YMAX_BOX])) {
        Hv->uold[IHvP(x, y, IP)] = one / H->dx / H->dx;
        printf("Lower corner test case : [%d] %d %d\n", H->mype, x, y);
      }
    }
  }
  if (H->testCase == 2) {
    if (H->nproc == 1) {
      x = ExtraLayer;
      y = ExtraLayer;
      for (j = y; j < H->jmax; j++) {
        Hv->uold[IHvP(x, j, IP)] = one / H->dx / H->dx;
      }
      printf("SOD tube test case\n");
    } else {
      x = ExtraLayer;
      y = ExtraLayer;
      for (j = 0; j < H->globny; j++) {
        if ((x >= H->box[XMIN_BOX]) && (x < H->box[XMAX_BOX]) && (j >= H->box[YMIN_BOX]) && (j < H->box[YMAX_BOX])) {
          y = j - H->box[YMIN_BOX] + ExtraLayer;
          Hv->uold[IHvP(x, y, IP)] = one / H->dx / H->dx;
        }
      }
      printf("SOD tube test case in //\n");
    }
  }
  if (H->testCase > 2) {
    printf("Test case not implemented -- aborting !\n");
    abort();
  }
  fflush(stdout);
}                               // hydro_init

void
hydro_finish(const hydroparam_t H, hydrovar_t * Hv) {
  Free(Hv->uold);
}                               // hydro_finish

void
allocate_work_space(int ngrid, const hydroparam_t H, hydrowork_t * Hw, hydrovarwork_t * Hvw) {
  int domain = ngrid * H.nxystep;
  int domainVar = domain * H.nvar;
  int domainD = domain * sizeof(double);
  int domainI = domain * sizeof(int);
  int domainVarD = domainVar * sizeof(double);

  WHERE("allocate_work_space");
  Hvw->u      = DMalloc(domainVar);
  Hvw->q      = DMalloc(domainVar);
  Hvw->dq     = DMalloc(domainVar);
  Hvw->qxm    = DMalloc(domainVar);
  Hvw->qxp    = DMalloc(domainVar);
  Hvw->qleft  = DMalloc(domainVar);
  Hvw->qright = DMalloc(domainVar);
  Hvw->qgdnv  = DMalloc(domainVar);
  Hvw->flux   = DMalloc(domainVar);
  Hw->e       = DMalloc(domain);
  Hw->c       = DMalloc(domain);

  Hw->sgnm = IMalloc(domain);

  Hw->pstar = DMalloc(domain);
  Hw->rl    = DMalloc(domain);
  Hw->ul    = DMalloc(domain);
  Hw->pl    = DMalloc(domain);
  Hw->cl    = DMalloc(domain);
  Hw->rr    = DMalloc(domain);
  Hw->ur    = DMalloc(domain);
  Hw->pr    = DMalloc(domain);
  Hw->cr    = DMalloc(domain);
  Hw->ro    = DMalloc(domain);
  Hw->goon  = IMalloc(domain);

  //   Hw->uo = DMalloc(ngrid);
  //   Hw->po = DMalloc(ngrid);
  //   Hw->co = DMalloc(ngrid);
  //   Hw->rstar = DMalloc(ngrid);
  //   Hw->ustar = DMalloc(ngrid);
  //   Hw->cstar = DMalloc(ngrid);
  //   Hw->wl = DMalloc(ngrid);
  //   Hw->wr = DMalloc(ngrid);
  //   Hw->wo = DMalloc((ngrid));
  //   Hw->spin = DMalloc(ngrid);
  //   Hw->spout = DMalloc(ngrid);
  //   Hw->ushock = DMalloc(ngrid);
  //   Hw->frac = DMalloc(ngrid);
  //   Hw->scr = DMalloc(ngrid);
  //   Hw->delp = DMalloc(ngrid);
  //   Hw->pold = DMalloc(ngrid);
  //   Hw->ind = IMalloc(ngrid);
  //   Hw->ind2 = IMalloc(ngrid);
}                               // allocate_work_space

void
deallocate_work_space(const hydroparam_t H, hydrowork_t * Hw, hydrovarwork_t * Hvw) {
  WHERE("deallocate_work_space");

  //
  Free(Hw->e);
  Free(Hw->c);
  //
  Free(Hvw->u);
  Free(Hvw->q);
  Free(Hvw->dq);
  Free(Hvw->qxm);
  Free(Hvw->qxp);
  Free(Hvw->qleft);
  Free(Hvw->qright);
  Free(Hvw->qgdnv);
  Free(Hvw->flux);
  Free(Hw->sgnm);

  //
  Free(Hw->pstar);
  Free(Hw->rl);
  Free(Hw->ul);
  Free(Hw->pl);
  Free(Hw->cl);
  Free(Hw->rr);
  Free(Hw->ur);
  Free(Hw->pr);
  Free(Hw->cr);
  Free(Hw->ro);
  Free(Hw->goon);
  //   Free(Hw->uo);
  //   Free(Hw->po);
  //   Free(Hw->co);
  //   Free(Hw->rstar);
  //   Free(Hw->ustar);
  //   Free(Hw->cstar);
  //   Free(Hw->wl);
  //   Free(Hw->wr);
  //   Free(Hw->wo);
  //   Free(Hw->spin);
  //   Free(Hw->spout);
  //   Free(Hw->ushock);
  //   Free(Hw->frac);
  //   Free(Hw->scr);
  //   Free(Hw->delp);
  //   Free(Hw->pold);
  //   Free(Hw->ind);
  //   Free(Hw->ind2);
}                               // deallocate_work_space


// EOF
