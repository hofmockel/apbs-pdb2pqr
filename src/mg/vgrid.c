/**
 *  @file    vgrid.c
 *  @author  Nathan Baker
 *  @brief   Class Vgrid methods
 *  @ingroup Vgrid
 *  @version $Id$
 *  @attention
 *  @verbatim 
 *
 * APBS -- Adaptive Poisson-Boltzmann Solver
 *
 * Nathan A. Baker (baker@biochem.wustl.edu)
 * Dept. of Biochemistry and Molecular Biophysics
 * Center for Computational Biology
 * Washington University in St. Louis
 *
 * Additional contributing authors listed in the code documentation.
 *
 * Copyright (c) 2002-2004.  Washington University in St. Louis.
 * All Rights Reserved.
 * Portions Copyright (c) 1999-2002.  The Regents of the University of
 * California.  
 * Portions Copyright (c) 1995.  Michael Holst.
 *
 * This file is part of APBS.
 *
 * APBS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * APBS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with APBS; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * @endverbatim
 */

#include "apbscfg.h"
#include "apbs/vgrid.h"

VEMBED(rcsid="$Id$")

#if !defined(VINLINE_VGRID)
    VPUBLIC unsigned long int Vgrid_memChk(Vgrid *thee) { 
        return Vmem_bytes(thee->mem); 
    }
#endif
#define IJK(i,j,k)  (((k)*(nx)*(ny))+((j)*(nx))+(i))

VPRIVATE char *MCwhiteChars = " =,;\t\n";
VPRIVATE char *MCcommChars  = "#%";
VPRIVATE double Vcompare;
VPRIVATE char Vprecision[26];

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vgrid_ctor
// Author:   Nathan Baker
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC Vgrid* Vgrid_ctor(int nx, int ny, int nz,
                  double hx, double hy, double hzed,
                  double xmin, double ymin, double zmin,
                  double *data) {

    Vgrid *thee = VNULL;

    thee = Vmem_malloc(VNULL, 1, sizeof(Vgrid));
    VASSERT(thee != VNULL);
    VASSERT(Vgrid_ctor2(thee, nx, ny, nz, hx, hy, hzed,
                  xmin, ymin, zmin, data));

    return thee;
}

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vgrid_ctor2
// Author:   Nathan Baker
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC int Vgrid_ctor2(Vgrid *thee, int nx, int ny, int nz,
                  double hx, double hy, double hzed,
                  double xmin, double ymin, double zmin,
                  double *data) {

    if (thee == VNULL) return 0;
    thee->nx = nx;
    thee->ny = ny;
    thee->nz = nz;
    thee->hx = hx;
    thee->hy = hy;
    thee->hzed = hzed;
    thee->xmin = xmin;
    thee->xmax = xmin + (nx-1)*hx;
    thee->ymin = ymin;
    thee->ymax = ymin + (ny-1)*hy;
    thee->zmin = zmin;
    thee->zmax = zmin + (nz-1)*hzed;
    if (data == VNULL) {
        thee->ctordata = 0;
        thee->readdata = 0;
    } else {
        thee->ctordata = 1;
        thee->readdata = 0;
        thee->data = data;
    }

    thee->mem = Vmem_ctor("APBS:VGRID");

    Vcompare = pow(10,-1*(VGRID_DIGITS - 2));
    sprintf(Vprecision,"%%12.%de %%12.%de %%12.%de", VGRID_DIGITS,
            VGRID_DIGITS, VGRID_DIGITS);

    return 1;
}

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vgrid_dtor
// Author:   Nathan Baker
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC void Vgrid_dtor(Vgrid **thee) {

    if ((*thee) != VNULL) {
        Vgrid_dtor2(*thee);
        Vmem_free(VNULL, 1, sizeof(Vgrid), (void **)thee);
        (*thee) = VNULL;
    }
}

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vgrid_dtor2
// Author:   Nathan Baker
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC void Vgrid_dtor2(Vgrid *thee) { 

    if (thee->readdata) {
        Vmem_free(thee->mem, (thee->nx*thee->ny*thee->nz), sizeof(double),
          (void **)&(thee->data));
    }
    Vmem_dtor(&(thee->mem));
 
}

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vgrid_value
// Author:   Nathan Baker
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC int Vgrid_value(Vgrid *thee, double pt[3], double *value) {

    int nx, ny, nz, ihi, jhi, khi, ilo, jlo, klo;
    double hx, hy, hzed, xmin, ymin, zmin, ifloat, jfloat, kfloat;
    double xmax, ymax, zmax;
    double u, dx, dy, dz;

    if (thee == VNULL) {
        Vnm_print(2, "Vgrid_value:  Error -- got VNULL thee!\n");
        VASSERT(0);
    }
    if (!(thee->ctordata || thee->readdata)) {
        Vnm_print(2, "Vgrid_value:  Error -- no data available!\n");
        VASSERT(0);
    }

    nx = thee->nx;
    ny = thee->ny;
    nz = thee->nz;
    hx = thee->hx;
    hy = thee->hy;
    hzed = thee->hzed;
    xmin = thee->xmin;
    ymin = thee->ymin;
    zmin = thee->zmin;
    xmax = thee->xmax;
    ymax = thee->ymax;
    zmax = thee->zmax;

    u = 0;
   
    ifloat = (pt[0] - xmin)/hx;
    jfloat = (pt[1] - ymin)/hy;
    kfloat = (pt[2] - zmin)/hzed;

    ihi = (int)ceil(ifloat);
    jhi = (int)ceil(jfloat);
    khi = (int)ceil(kfloat);
    ilo = (int)floor(ifloat);
    jlo = (int)floor(jfloat);
    klo = (int)floor(kfloat);
	if (VABS(pt[0] - xmin) < Vcompare) ilo = 0;
	if (VABS(pt[1] - ymin) < Vcompare) jlo = 0;
	if (VABS(pt[2] - zmin) < Vcompare) klo = 0;
	if (VABS(pt[0] - xmax) < Vcompare) ihi = nx-1;
	if (VABS(pt[1] - ymax) < Vcompare) jhi = ny-1;
	if (VABS(pt[2] - zmax) < Vcompare) khi = nz-1;

    /* See if we're on the mesh */
    if ((ihi<nx) && (jhi<ny) && (khi<nz) &&
        (ilo>=0) && (jlo>=0) && (klo>=0)) {

        dx = ifloat - (double)(ilo);
        dy = jfloat - (double)(jlo);
        dz = kfloat - (double)(klo);
        u = dx      *dy      *dz      *(thee->data[IJK(ihi,jhi,khi)])
          + dx      *(1.0-dy)*dz      *(thee->data[IJK(ihi,jlo,khi)])
          + dx      *dy      *(1.0-dz)*(thee->data[IJK(ihi,jhi,klo)])
          + dx      *(1.0-dy)*(1.0-dz)*(thee->data[IJK(ihi,jlo,klo)])
          + (1.0-dx)*dy      *dz      *(thee->data[IJK(ilo,jhi,khi)])
          + (1.0-dx)*(1.0-dy)*dz      *(thee->data[IJK(ilo,jlo,khi)])
          + (1.0-dx)*dy      *(1.0-dz)*(thee->data[IJK(ilo,jhi,klo)])
          + (1.0-dx)*(1.0-dy)*(1.0-dz)*(thee->data[IJK(ilo,jlo,klo)]);

        *value = u;
        if (isnan(u)) {
            Vnm_print(2, "Vgrid_value:  Got NaN!\n");
            Vnm_print(2, "Vgrid_value:  (x, y, z) = (%4.3f, %4.3f, %4.3f)\n", 
                    pt[0], pt[1], pt[2]);
            Vnm_print(2, "Vgrid_value:  (ihi, jhi, khi) = (%d, %d, %d)\n", 
                    ihi, jhi, khi);
            Vnm_print(2, "Vgrid_value:  (ilo, jlo, klo) = (%d, %d, %d)\n", 
                    ilo, jlo, klo);
            Vnm_print(2, "Vgrid_value:  (nx, ny, nz) = (%d, %d, %d)\n", 
                    nx, ny, nz);
            Vnm_print(2, "Vgrid_value:  (dx, dy, dz) = (%4.3f, %4.3f, %4.3f)\n", 
                    dx, dy, dz);
            Vnm_print(2, "Vgrid_value:  data[IJK(ihi,jhi,khi)] = %g\n", 
                    thee->data[IJK(ihi,jhi,khi)]);
            Vnm_print(2, "Vgrid_value:  data[IJK(ihi,jlo,khi)] = %g\n", 
                    thee->data[IJK(ihi,jlo,khi)]);
            Vnm_print(2, "Vgrid_value:  data[IJK(ihi,jhi,klo)] = %g\n", 
                    thee->data[IJK(ihi,jhi,klo)]);
            Vnm_print(2, "Vgrid_value:  data[IJK(ihi,jlo,klo)] = %g\n", 
                    thee->data[IJK(ihi,jlo,klo)]);
            Vnm_print(2, "Vgrid_value:  data[IJK(ilo,jhi,khi)] = %g\n", 
                    thee->data[IJK(ilo,jhi,khi)]);
            Vnm_print(2, "Vgrid_value:  data[IJK(ilo,jlo,khi)] = %g\n", 
                    thee->data[IJK(ilo,jlo,khi)]);
            Vnm_print(2, "Vgrid_value:  data[IJK(ilo,jhi,klo)] = %g\n", 
                    thee->data[IJK(ilo,jhi,klo)]);
            Vnm_print(2, "Vgrid_value:  data[IJK(ilo,jlo,klo)] = %g\n", 
                    thee->data[IJK(ilo,jlo,klo)]);
        }
        return 1;

    } else {

        *value = 0;
        return 0;

    }

    return 0;

}

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vgrid_curvature
//
//   Notes:  cflag=0 ==> Reduced Maximal Curvature
//           cflag=1 ==> Mean Curvature (Laplace)
//           cflag=2 ==> Gauss Curvature
//           cflag=3 ==> True Maximal Curvature
//
// Authors:  Stephen Bond and Nathan Baker
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC int Vgrid_curvature(Vgrid *thee, double pt[3], int cflag, 
  double *value) {

    double hx, hy, hzed, curv;
    double dxx, dyy, dzz;
    double uleft, umid, uright, testpt[3];

    if (thee == VNULL) {
        Vnm_print(2, "Vgrid_curvature:  Error -- got VNULL thee!\n");
        VASSERT(0);
    }
    if (!(thee->ctordata || thee->readdata)) {
        Vnm_print(2, "Vgrid_curvature:  Error -- no data available!\n");
        VASSERT(0);
    }

    hx = thee->hx;
    hy = thee->hy;
    hzed = thee->hzed;

    curv = 0.0;

    testpt[0] = pt[0];
    testpt[1] = pt[1];
    testpt[2] = pt[2];

    /* Compute 2nd derivative in the x-direction */
    VJMPERR1(Vgrid_value( thee, testpt, &umid));
    testpt[0] = pt[0] - hx;
    VJMPERR1(Vgrid_value( thee, testpt, &uleft));
    testpt[0] = pt[0] + hx;
    VJMPERR1(Vgrid_value( thee, testpt, &uright));
    testpt[0] = pt[0];

    dxx = (uright - 2*umid + uleft)/(hx*hx);

    /* Compute 2nd derivative in the y-direction */
    VJMPERR1(Vgrid_value( thee, testpt, &umid));
    testpt[1] = pt[1] - hy;
    VJMPERR1(Vgrid_value( thee, testpt, &uleft));
    testpt[1] = pt[1] + hy;
    VJMPERR1(Vgrid_value( thee, testpt, &uright));
    testpt[1] = pt[1];

    dyy = (uright - 2*umid + uleft)/(hy*hy);

    /* Compute 2nd derivative in the z-direction */
    VJMPERR1(Vgrid_value( thee, testpt, &umid));
    testpt[2] = pt[2] - hzed;
    VJMPERR1(Vgrid_value( thee, testpt, &uleft));
    testpt[2] = pt[2] + hzed;
    VJMPERR1(Vgrid_value( thee, testpt, &uright));

    dzz = (uright - 2*umid + uleft)/(hzed*hzed);


    if ( cflag == 0 ) {
        curv = fabs(dxx);
        curv = ( curv > fabs(dyy) ) ? curv : fabs(dyy);
        curv = ( curv > fabs(dzz) ) ? curv : fabs(dzz);
    } else if ( cflag == 1 ) {
        curv = (dxx + dyy + dzz)/3.0;
    } else {
        Vnm_print(2, "Vgrid_curvature:  support for cflag = %d not available!\n", cflag);
        VASSERT( 0 ); /* Feature Not Coded Yet! */
    }

    *value = curv;
    return 1;

    VERROR1:
        return 0; 

}

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vgrid_gradient
//
// Authors:  Nathan Baker and Stephen Bond
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC int Vgrid_gradient(Vgrid *thee, double pt[3], double grad[3]) {

    double hx, hy, hzed;
    double uleft, umid, uright, testpt[3];
    int haveleft, haveright;

    if (thee == VNULL) {
        Vnm_print(2, "Vgrid_gradient:  Error -- got VNULL thee!\n");
        VASSERT(0);
    }
    if (!(thee->ctordata || thee->readdata)) {
        Vnm_print(2, "Vgrid_gradient:  Error -- no data available!\n");
        VASSERT(0);
    }

    hx = thee->hx;
    hy = thee->hy;
    hzed = thee->hzed;

    /* Compute derivative in the x-direction */
    testpt[0] = pt[0];
    testpt[1] = pt[1];
    testpt[2] = pt[2];
    VJMPERR1( Vgrid_value( thee, testpt, &umid));
    testpt[0] = pt[0] - hx;
    if (Vgrid_value( thee, testpt, &uleft)) haveleft = 1;
    else haveleft = 0;
    testpt[0] = pt[0] + hx;
    if (Vgrid_value( thee, testpt, &uright)) haveright = 1;
    else haveright = 0;
    if (haveright && haveleft) grad[0] = (uright - uleft)/(2*hx);
    else if (haveright) grad[0] = (uright - umid)/hx;
    else if (haveleft) grad[0] = (umid - uleft)/hx;
    else VJMPERR1(0);

    /* Compute derivative in the y-direction */
    testpt[0] = pt[0];
    testpt[1] = pt[1];
    testpt[2] = pt[2];
    VJMPERR1(Vgrid_value(thee, testpt, &umid));
    testpt[1] = pt[1] - hy;
    if (Vgrid_value( thee, testpt, &uleft)) haveleft = 1;
    else haveleft = 0;
    testpt[1] = pt[1] + hy;
    if (Vgrid_value( thee, testpt, &uright)) haveright = 1;
    else haveright = 0;
    if (haveright && haveleft) grad[1] = (uright - uleft)/(2*hy);
    else if (haveright) grad[1] = (uright - umid)/hy;
    else if (haveleft) grad[1] = (umid - uleft)/hy;
    else VJMPERR1(0);

    /* Compute derivative in the z-direction */
    testpt[0] = pt[0];
    testpt[1] = pt[1];
    testpt[2] = pt[2];
    VJMPERR1(Vgrid_value(thee, testpt, &umid));
    testpt[2] = pt[2] - hzed;
    if (Vgrid_value( thee, testpt, &uleft)) haveleft = 1;
    else haveleft = 0;
    testpt[2] = pt[2] + hzed;
    if (Vgrid_value( thee, testpt, &uright)) haveright = 1;
    else haveright = 0;
    if (haveright && haveleft) grad[2] = (uright - uleft)/(2*hzed);
    else if (haveright) grad[2] = (uright - umid)/hzed;
    else if (haveleft) grad[2] = (umid - uleft)/hzed;
    else VJMPERR1(0);

    return 1;

    VERROR1:
        return 0; 

}

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vgrid_readDX
//
// Author:   Nathan Baker
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC int Vgrid_readDX(Vgrid *thee, const char *iodev, const char *iofmt,
  const char *thost, const char *fname) {

    int i, j, k, itmp, u;
    double dtmp;
    char tok[VMAX_BUFSIZE];
    Vio *sock;

    /* Check to see if the existing data is null and, if not, clear it out */
    if (thee->data != VNULL) {
        Vnm_print(1, "Vgrid_readDX:  destroying existing data!\n");
	Vmem_free(thee->mem, (thee->nx*thee->ny*thee->nz), sizeof(double),
          (void **)&(thee->data)); }
    thee->readdata = 1;
    thee->ctordata = 0;

    /* Set up the virtual socket */
    sock = Vio_ctor(iodev,iofmt,thost,fname,"r");
    if (sock == VNULL) {
        Vnm_print(2, "Vgrid_readDX: Problem opening virtual socket %s\n",
          fname);
        return 0;
    }
    if (Vio_accept(sock, 0) < 0) {
        Vnm_print(2, "Vgrid_readDX: Problem accepting virtual socket %s\n",
          fname);
        return 0;
    }

    Vio_setWhiteChars(sock, MCwhiteChars);
    Vio_setCommChars(sock, MCcommChars);

    /* Read in the DX regular positions */
    /* Get "object" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "object"));
    /* Get "1" */
    VJMPERR2(1 == Vio_scanf(sock, "%d", &itmp));
    /* Get "class" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "class"));
    /* Get "gridpositions" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "gridpositions"));
    /* Get "counts" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "counts"));
    /* Get nx */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%d", &(thee->nx)));
    /* Get ny */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%d", &(thee->ny)));
    /* Get nz */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%d", &(thee->nz)));
    Vnm_print(0, "Vgrid_readDX:  Grid dimensions %d x %d x %d grid\n", 
     thee->nx, thee->ny, thee->nz);
    /* Get "origin" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "origin"));
    /* Get xmin */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%lf", &(thee->xmin)));
    /* Get ymin */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%lf", &(thee->ymin)));
    /* Get zmin */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%lf", &(thee->zmin)));
    Vnm_print(0, "Vgrid_readDX:  Grid origin = (%g, %g, %g)\n",
      thee->xmin, thee->ymin, thee->zmin);
    /* Get "delta" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "delta"));
    /* Get hx */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%lf", &(thee->hx)));
    /* Get 0.0 */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%lf", &dtmp));
    VJMPERR1(dtmp == 0.0);
    /* Get 0.0 */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%lf", &dtmp));
    VJMPERR1(dtmp == 0.0);
    /* Get "delta" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "delta"));
    /* Get 0.0 */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%lf", &dtmp));
    VJMPERR1(dtmp == 0.0);
    /* Get hy */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%lf", &(thee->hy)));
    /* Get 0.0 */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%lf", &dtmp));
    VJMPERR1(dtmp == 0.0);
    /* Get "delta" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "delta"));
    /* Get 0.0 */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%lf", &dtmp));
    VJMPERR1(dtmp == 0.0);
    /* Get 0.0 */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%lf", &dtmp));
    VJMPERR1(dtmp == 0.0);
    /* Get hz */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%lf", &(thee->hzed)));
    Vnm_print(0, "Vgrid_readDX:  Grid spacings = (%g, %g, %g)\n",
      thee->hx, thee->hy, thee->hzed);
    /* Get "object" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "object"));
    /* Get "2" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    /* Get "class" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "class"));
    /* Get "gridconnections" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "gridconnections"));
    /* Get "counts" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "counts"));
    /* Get the dimensions again */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    /* Get "object" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "object"));
    /* Get # */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    /* Get "class" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "class"));
    /* Get "array" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "array"));
    /* Get "type" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "type"));
    /* Get "double" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "double"));
    /* Get "rank" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "rank"));
    /* Get # */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    /* Get "items" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "items"));
    /* Get # */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(1 == sscanf(tok, "%d", &itmp));
    VJMPERR1(((thee->nx)*(thee->ny)*(thee->nz)) == itmp);
    /* Get "data" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "data"));
    /* Get "follows" */
    VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
    VJMPERR1(!strcmp(tok, "follows"));

    /* Allocate space for the data */
    Vnm_print(0, "Vgrid_readDX:  allocating %d x %d x %d doubles for storage\n",
      thee->nx, thee->ny, thee->nz);
    thee->data = VNULL;
    thee->data = Vmem_malloc(thee->mem, (thee->nx)*(thee->ny)*(thee->nz), 
      sizeof(double));
    if (thee->data == VNULL) {
        Vnm_print(2, "Vgrid_readDX:  Unable to allocate space for data!\n");
        return 0;
    }
                     
    for (i=0; i<thee->nx; i++) {
        for (j=0; j<thee->ny; j++) {
            for (k=0; k<thee->nz; k++) {
                u = k*(thee->nx)*(thee->ny)+j*(thee->nx)+i;
                VJMPERR2(1 == Vio_scanf(sock, "%s", tok));
                VJMPERR1(1 == sscanf(tok, "%lf", &dtmp));
                (thee->data)[u] = dtmp;
            }
        }
    }
	 
    /* calculate grid maxima */
    thee->xmax = thee->xmin + (thee->nx-1)*thee->hx;
    thee->ymax = thee->ymin + (thee->ny-1)*thee->hy;
    thee->zmax = thee->zmin + (thee->nz-1)*thee->hzed;

    /* Close off the socket */
    Vio_acceptFree(sock);
    Vio_dtor(&sock);

    return 1;

  VERROR1:
    Vio_dtor(&sock);
    Vnm_print(2, "Vgrid_readDX:  Format problem with input file <%s>\n",
      fname);
    return 0;

  VERROR2:
    Vio_dtor(&sock);
    Vnm_print(2, "Vgrid_readDX:  I/O problem with input file <%s>\n",
      fname);
    return 0;



}

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vgrid_writeDX
//
// Author:   Nathan Baker
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC void Vgrid_writeDX(Vgrid *thee, const char *iodev, const char *iofmt,
  const char *thost, const char *fname, char *title, double *pvec) {

    double xmin, ymin, zmin, hx, hy, hzed;
    int nx, ny, nz;
    int icol, i, j, k, u, usepart, nxPART, nyPART, nzPART, gotit;
    double x, y, z, xminPART, yminPART, zminPART;
    Vio *sock;
    char precFormat[VMAX_BUFSIZE];

    if (thee == VNULL) {
        Vnm_print(2, "Vgrid_writeDX:  Error -- got VNULL thee!\n");
        VASSERT(0);
    }
    if (!(thee->ctordata || thee->readdata)) {
        Vnm_print(2, "Vgrid_writeDX:  Error -- no data available!\n");
        VASSERT(0);
    }

    hx = thee->hx;
    hy = thee->hy; 
    hzed = thee->hzed; 
    nx = thee->nx;
    ny = thee->ny;
    nz = thee->nz;
    xmin = thee->xmin;
    ymin = thee->ymin;
    zmin = thee->zmin;

    if (pvec == VNULL) usepart = 0;
    else usepart = 1;

    /* Set up the virtual socket */
    Vnm_print(0, "Vgrid_writeDX:  Opening virtual socket...\n");
    sock = Vio_ctor(iodev,iofmt,thost,fname,"w");
    if (sock == VNULL) {
        Vnm_print(2, "Vgrid_writeDX:  Problem opening virtual socket %s\n",
          fname);
        return;
    }
    if (Vio_connect(sock, 0) < 0) {
        Vnm_print(2, "Vgrid_writeDX: Problem connecting virtual socket %s\n",
          fname);
        return; 
    }

    Vio_setWhiteChars(sock, MCwhiteChars);
    Vio_setCommChars(sock, MCcommChars);

    Vnm_print(0, "Vgrid_writeDX:  Writing to virtual socket...\n");
    if (usepart) {
        /* Get the lower corner and number of grid points for the local
         * partition */
        xminPART = VLARGE;
        yminPART = VLARGE;
        zminPART = VLARGE;
        nxPART = 0;
        nyPART = 0;
        nzPART = 0;
        /* First, search for the lower corner */
        for (k=0; k<nz; k++) {
            z = k*hzed + zmin;
            for (j=0; j<ny; j++) {
                y = j*hy + ymin;
                for (i=0; i<nx; i++) {
                    x = i*hx + xmin;
                    if (pvec[IJK(i,j,k)] > 0.0) {
                        if (x < xminPART) xminPART = x;
                        if (y < yminPART) yminPART = y;
                        if (z < zminPART) zminPART = z;
                    }
                }
            }
        }
        /* Now search for the number of grid points in the z direction */
        for (k=0; k<nz; k++) {
            gotit = 0;
            for (j=0; j<ny; j++) {
                for (i=0; i<nx; i++) {
                    if (pvec[IJK(i,j,k)] > 0.0) {
                        gotit = 1;
                        break;
                    }
                }
                if (gotit) break;
            }
            if (gotit) nzPART++;
        }
        /* Now search for the number of grid points in the y direction */
        for (j=0; j<ny; j++) {
            gotit = 0;
            for (k=0; k<nz; k++) {
                for (i=0; i<nx; i++) {
                    if (pvec[IJK(i,j,k)] > 0.0) {
                        gotit = 1;
                        break;
                    }
                }
                if (gotit) break;
            }
            if (gotit) nyPART++;
        }
        /* Now search for the number of grid points in the x direction */
        for (i=0; i<nx; i++) {
            gotit = 0;
            for (k=0; k<nz; k++) {
                for (j=0; j<ny; j++) {
                    if (pvec[IJK(i,j,k)] > 0.0) {
                        gotit = 1;
                        break; 
                    }
                }
                if (gotit) break;
            }
            if (gotit) nxPART++;
        }

        if ((nxPART != nx) || (nyPART != ny) || (nzPART != nz)) {
            Vnm_print(0, "Vgrid_writeDX:  printing only subset of domain\n");
        }


        /* Write off the title (if we're not XDR) */
        if (Vstring_strcasecmp(iofmt, "XDR") == 0) {
            Vnm_print(0, "Vgrid_writeDX:  Skipping comments for XDR format.\n");
        } else {
            Vnm_print(0, "Vgrid_writeDX:  Writing comments for %s format.\n",
              iofmt);
            Vio_printf(sock, "# Data from %s\n", PACKAGE_STRING);
            Vio_printf(sock, "# \n");
            Vio_printf(sock, "# %s\n", title);
            Vio_printf(sock, "# \n");
        }

        /* Write off the DX regular positions */
        Vio_printf(sock, "object 1 class gridpositions counts %d %d %d\n",
          nxPART, nyPART, nzPART);
        
        sprintf(precFormat, Vprecision, xminPART, yminPART, zminPART);
        Vio_printf(sock, "origin %s\n", precFormat);
        sprintf(precFormat, Vprecision, hx, 0.0, 0.0);
        Vio_printf(sock, "delta %s\n", precFormat);
        sprintf(precFormat, Vprecision, 0.0, hy, 0.0);
        Vio_printf(sock, "delta %s\n", precFormat);
        sprintf(precFormat, Vprecision, 0.0, 0.0, hzed);
        Vio_printf(sock, "delta %s\n", precFormat);
        
        /* Write off the DX regular connections */
        Vio_printf(sock, "object 2 class gridconnections counts %d %d %d\n",
          nxPART, nyPART, nzPART);
 
        /* Write off the DX data */
        Vio_printf(sock, "object 3 class array type double rank 0 items %d \
data follows\n", (nxPART*nyPART*nzPART));
        icol = 0;
        for (i=0; i<nx; i++) {
            for (j=0; j<ny; j++) {
                for (k=0; k<nz; k++) {
                    u = k*(nx)*(ny)+j*(nx)+i;
                    if (pvec[u] > 0.0) {
                        Vio_printf(sock, "%12.6e ", thee->data[u]);
                        icol++;
                        if (icol == 3) {
                            icol = 0;
                            Vio_printf(sock, "\n");
                        }
                    }
                }
            }
        }

        if (icol != 0) Vio_printf(sock, "\n");

        /* Create the field */
        Vio_printf(sock, "attribute \"dep\" string \"positions\"\n");
        Vio_printf(sock, "object \"regular positions regular connections\" \
class field\n");
        Vio_printf(sock, "component \"positions\" value 1\n");
        Vio_printf(sock, "component \"connections\" value 2\n");
        Vio_printf(sock, "component \"data\" value 3\n");

    } else {
        /* Write off the title (if we're not XDR) */
        if (Vstring_strcasecmp(iofmt, "XDR") == 0) {
            Vnm_print(0, "Vgrid_writeDX:  Skipping comments for XDR format.\n");
        } else {
            Vnm_print(0, "Vgrid_writeDX:  Writing comments for %s format.\n",
              iofmt);
            Vio_printf(sock, "# Data from %s\n", PACKAGE_STRING);
            Vio_printf(sock, "# \n");
            Vio_printf(sock, "# %s\n", title);
            Vio_printf(sock, "# \n");
        }


        /* Write off the DX regular positions */
        Vio_printf(sock, "object 1 class gridpositions counts %d %d %d\n",
          nx, ny, nz);

        sprintf(precFormat, Vprecision, xminPART, yminPART, zminPART);
        Vio_printf(sock, "origin %s\n", precFormat);
        sprintf(precFormat, Vprecision, hx, 0.0, 0.0);
        Vio_printf(sock, "delta %s\n", precFormat);
        sprintf(precFormat, Vprecision, 0.0, hy, 0.0);
        Vio_printf(sock, "delta %s\n", precFormat);
        sprintf(precFormat, Vprecision, 0.0, 0.0, hzed);
        Vio_printf(sock, "delta %s\n", precFormat);
    
        /* Write off the DX regular connections */
        Vio_printf(sock, "object 2 class gridconnections counts %d %d %d\n",
          nx, ny, nz);

        /* Write off the DX data */
        Vio_printf(sock, "object 3 class array type double rank 0 items %d \
data follows\n", (nx*ny*nz));
        icol = 0;
        for (i=0; i<nx; i++) {
            for (j=0; j<ny; j++) { 
                for (k=0; k<nz; k++) {
                    u = k*(nx)*(ny)+j*(nx)+i;
                    Vio_printf(sock, "%12.6e ", thee->data[u]);
                    icol++;
                    if (icol == 3) {
                        icol = 0;
                        Vio_printf(sock, "\n");
                    }
                }
            }
        }
        if (icol != 0) Vio_printf(sock, "\n");
           
        /* Create the field */
        Vio_printf(sock, "attribute \"dep\" string \"positions\"\n");
        Vio_printf(sock, "object \"regular positions regular connections\" \
class field\n");
        Vio_printf(sock, "component \"positions\" value 1\n");
        Vio_printf(sock, "component \"connections\" value 2\n");
        Vio_printf(sock, "component \"data\" value 3\n");
    }

    /* Close off the socket */
    Vio_connectFree(sock);
    Vio_dtor(&sock);
}

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vgrid_writeUHBD
// Author:   Nathan Baker
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC void Vgrid_writeUHBD(Vgrid *thee, const char *iodev, const char *iofmt,
  const char *thost, const char *fname, char *title, double *pvec) {

    int icol, i, j, k, u, nx, ny, nz, gotit;
    double xmin, ymin, zmin, hzed, hy, hx;
    Vio *sock;

    if (thee == VNULL) {
        Vnm_print(2, "Vgrid_writeUHBD:  Error -- got VNULL thee!\n");
        VASSERT(0);
    }
    if (!(thee->ctordata || thee->readdata)) {
        Vnm_print(2, "Vgrid_writeUHBD:  Error -- no data available!\n");
        VASSERT(0);
    }

    if ((thee->hx!=thee->hy) || (thee->hy!=thee->hzed)
      || (thee->hx!=thee->hzed)) {
        Vnm_print(2, "Vgrid_writeUHBD: can't write UHBD mesh with non-uniform \
spacing\n");
        return;
    }

    /* Set up the virtual socket */
    sock = Vio_ctor(iodev,iofmt,thost,fname,"w");
    if (sock == VNULL) {
        Vnm_print(2, "Vgrid_writeUHBD: Problem opening virtual socket %s\n",
          fname);
        return;
    }
    if (Vio_connect(sock, 0) < 0) {
        Vnm_print(2, "Vgrid_writeUHBD: Problem connecting virtual socket %s\n",
          fname);
        return;
    }

    /* Get the lower corner and number of grid points for the local
     * partition */
    hx = thee->hx;
    hy = thee->hy;
    hzed = thee->hzed;
    nx = thee->nx;
    ny = thee->ny;
    nz = thee->nz;
    xmin = thee->xmin;
    ymin = thee->ymin;
    zmin = thee->zmin;

    /* Let interested folks know that partition information is ignored */
    if (pvec != VNULL) {
        gotit = 0;
        for (i=0; i<(nx*ny*nz); i++) {
            if (pvec[i] == 0) {
                gotit = 1;
                break;
            }
        }
        if (gotit) {
            Vnm_print(2, "Vgrid_writeUHBD:  IGNORING PARTITION INFORMATION!\n");
            Vnm_print(2, "Vgrid_writeUHBD:  This means I/O from parallel runs \
will have significant overlap.\n");
        }
    }

    /* Write out the header */
    Vio_printf(sock, "%72s\n", title);
    Vio_printf(sock, "%12.5e%12.5e%7d%7d%7d%7d%7d\n", 1.0, 0.0, -1, 0,
      nz, 1, nz);
    Vio_printf(sock, "%7d%7d%7d%12.5e%12.5e%12.5e%12.5e\n", nx, ny, nz,
      hx, (xmin-hx), (ymin-hx), (zmin-hx));
    Vio_printf(sock, "%12.5e%12.5e%12.5e%12.5e\n", 0.0, 0.0, 0.0, 0.0);
    Vio_printf(sock, "%12.5e%12.5e%7d%7d", 0.0, 0.0, 0, 0);

    /* Write out the entries */
    icol = 0; 
    for (k=0; k<nz; k++) {
        Vio_printf(sock, "\n%7d%7d%7d\n", k+1, thee->nx, thee->ny);
        icol = 0;
        for (j=0; j<ny; j++) {
            for (i=0; i<nx; i++) {
                u = k*(nx)*(ny)+j*(nx)+i;
                icol++;
                Vio_printf(sock, " %12.5e", thee->data[u]);
                if (icol == 6) {
                    icol = 0;
                    Vio_printf(sock, "\n");
                }
            }
        }
    }
    if (icol != 0) Vio_printf(sock, "\n");

    /* Close off the socket */
    Vio_connectFree(sock);
    Vio_dtor(&sock);
}

VPUBLIC double Vgrid_integrate(Vgrid *thee) {

    int i, j, k, nx, ny, nz;
    double sum;

    if (thee == VNULL) {
        Vnm_print(2, "Vgrid_integrate:  Got VNULL thee!\n");
        VASSERT(0);
    }

    nx = thee->nx;
    ny = thee->ny;
    nz = thee->nz;

    sum = 0.0;
    for (k=0; k<nz; k++) {
        for (j=0; j<ny; j++) {
            for (i=0; i<nx; i++) {
                sum = sum + thee->data[IJK(i,j,k)];
            }
        }
    }

    sum = sum*(thee->hx)*(thee->hy)*(thee->hzed);

    return sum;

}


VPUBLIC double Vgrid_normL1(Vgrid *thee) {

    int i, j, k, nx, ny, nz;
    double sum;

    if (thee == VNULL) {
        Vnm_print(2, "Vgrid_normL1:  Got VNULL thee!\n");
        VASSERT(0);
    }

    nx = thee->nx;
    ny = thee->ny;
    nz = thee->nz;

    sum = 0.0;
    for (k=0; k<nz; k++) {
        for (j=0; j<ny; j++) {
            for (i=0; i<nx; i++) {
                sum = sum + VABS(thee->data[IJK(i,j,k)]);
            }
        }
    }

    sum = sum*(thee->hx)*(thee->hy)*(thee->hzed);

    return sum;

}

VPUBLIC double Vgrid_normL2(Vgrid *thee) {

    int i, j, k, nx, ny, nz;
    double sum;

    if (thee == VNULL) {
        Vnm_print(2, "Vgrid_normL2:  Got VNULL thee!\n");
        VASSERT(0);
    }

    nx = thee->nx;
    ny = thee->ny;
    nz = thee->nz;

    sum = 0.0;
    for (k=0; k<nz; k++) {
        for (j=0; j<ny; j++) {
            for (i=0; i<nx; i++) {
                sum = sum + VSQR(thee->data[IJK(i,j,k)]);
            }
        }
    }

    sum = sum*(thee->hx)*(thee->hy)*(thee->hzed);

    return VSQRT(sum);

}

VPUBLIC double Vgrid_seminormH1(Vgrid *thee) {

    int i, j, k, d, nx, ny, nz;
    double pt[3], grad[3], sum, hx, hy, hzed, xmin, ymin, zmin;

    if (thee == VNULL) {
        Vnm_print(2, "Vgrid_seminormH1:  Got VNULL thee!\n");
        VASSERT(0);
    }

    nx = thee->nx;
    ny = thee->ny;
    nz = thee->nz;
    hx = thee->hx;
    hy = thee->hy;
    hzed = thee->hzed;
    xmin = thee->xmin;
    ymin = thee->ymin;
    zmin = thee->zmin;

    sum = 0.0;
    for (k=0; k<nz; k++) {
        pt[2] = k*hzed + zmin;
        for (j=0; j<ny; j++) {
            pt[1] = j*hy + ymin;
            for (i=0; i<nx; i++) {
                pt[0] = i*hx + xmin;
                VASSERT(Vgrid_gradient(thee, pt, grad));
                for (d=0; d<3; d++) sum = sum + VSQR(grad[d]);
            }
        }
    }

    sum = sum*(hx)*(hy)*(hzed);

    if (VABS(sum) < VSMALL) sum = 0.0;
    else sum = VSQRT(sum);

    return sum;

}

VPUBLIC double Vgrid_normH1(Vgrid *thee) {

    double sum = 0.0;

    if (thee == VNULL) {
        Vnm_print(2, "Vgrid_normH1:  Got VNULL thee!\n");
        VASSERT(0);
    }

    sum = VSQR(Vgrid_seminormH1(thee)) + VSQR(Vgrid_normL2(thee));

    return VSQRT(sum);

}

VPUBLIC double Vgrid_normLinf(Vgrid *thee) {

    int i, j, k, nx, ny, nz, gotval;
    double sum, val;

    if (thee == VNULL) {
        Vnm_print(2, "Vgrid_normLinf:  Got VNULL thee!\n");
        VASSERT(0);
    }

    nx = thee->nx;
    ny = thee->ny;
    nz = thee->nz;

    sum = 0.0;
    gotval = 0;
    for (k=0; k<nz; k++) {
        for (j=0; j<ny; j++) {
            for (i=0; i<nx; i++) {
                val = VABS(thee->data[IJK(i,j,k)]);
                if (!gotval) {
                    gotval = 1;
                    sum = val;
                } 
                if (val > sum) sum = val;
            }
        }
    }

    return sum;

}

