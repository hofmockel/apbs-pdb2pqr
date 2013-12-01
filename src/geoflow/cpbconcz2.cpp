///  @file    cpbconcz2.cpp 
///  @author  Peter Hui, Andrew Stevens, Kyle Monson, Zhan Chen, Guowei Wei
///  @brief this is the top level of geoflow
///  @ingroup Geoflow
///  @version $Id$
///  @attention
///  @verbatim
///
/// APBS -- Adaptive Poisson-Boltzmann Solver
///
///  Nathan A. Baker (nathan.baker@pnnl.gov)
///  Pacific Northwest National Laboratory
///
///  Additional contributing authors listed in the code documentation.
///
/// Copyright (c) 2010-2012 Battelle Memorial Institute. Developed at the
/// Pacific Northwest National Laboratory, operated by Battelle Memorial
/// Institute, Pacific Northwest Division for the U.S. Department of Energy.
///
/// Portions Copyright (c) 2002-2010, Washington University in St. Louis.
/// Portions Copyright (c) 2002-2010, Nathan A. Baker.
/// Portions Copyright (c) 1999-2002, The Regents of the University of
/// California.
/// Portions Copyright (c) 1995, Michael Holst.
/// All rights reserved.
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions are met:
///
/// Redistributions of source code must retain the above copyright notice, this
/// list of conditions and the following disclaimer.
///
/// Redistributions in binary form must reproduce the above copyright notice,
/// this list of conditions and the following disclaimer in the documentation
/// and/or other materials provided with the distribution.
///
/// Neither the name of the developer nor the names of its contributors may be
/// used to endorse or promote products derived from this software without
/// specific prior written permission.
///
/// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
/// THE POSSIBILITY OF SUCH DAMAGE.
///
/// @endverbatim


#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <stdlib.h>
#include "modules.h"
#include "cpbconcz2.h"

using namespace std;

void
initXYZR(double xyzr[MAXATOMS][XYZRWIDTH]) {
    for (int i = 0; i < XYZRWIDTH; i++) {
        for (int j = 0; j < MAXATOMS; j++) {
            xyzr[j][i] = 0.0;
        }
    }
}

void
writePressGamaRms() {
    std::ofstream f;
    std::string spaces("    ");
    f.open("press_gama_rms.txt");
    f << spaces << "pressure" << spaces << "gamma" << spaces << "rmse" << spaces << "rmse_error";
    f << std::endl;
    f.close();
}

void
writeOutputMolFiles(int nmol) {
    std::string spaces("    ");
    for (int i = 1; i <= nmol; i++) {
        std::ofstream f;
        std::stringstream s;
        s << "output_mol" << 9 + i << ".txt"; // e.g. output_mol10.txt
        f.open(s.str().c_str());
        f << spaces << "pressure" << spaces << "gamma" << spaces << "nonpolar" << spaces << "electro " << spaces << " totl_energy" << "  area" << spaces << "volume" << spaces << "uattint" << spaces << "expt" << spaces << "error";
        f << std::endl;
        f.close();
    }

}

void
writeSupportingFiles(int nmol) {
    writePressGamaRms();
    writeOutputMolFiles(nmol);
}

int
numberOfLines(std::string fileName) {
    std::ifstream file(fileName.c_str());
    return std::count(std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>(), '\n');
}

void
normalizeSurfuAndEps(Mat<>& surfu, Mat<>& eps, double epsilons, double epsilonp) {
    for (size_t i = 0; i < surfu.size(); i++) {
        if (surfu[i] > 1000.0) { surfu[i] = 1000.0; }
        if (surfu[i] < 0.0) { surfu[i] = 0.0; }
        eps[i] = epsilonp + (epsilons - epsilonp) * ( (1000.0 - surfu[i])/1000.0 );
    }
}

/*
 * Compute soleng1 and soleng2 (solvation).  This is an artifact of refactoring the original F90 code.
 * Parameters:
 *     soleng:        The soleng variable to set (compute).  For soleng1, the phi array should be phi; for 
 *             soleng2, the phiarray should be phivoc. 
 *     phi:        Either phi or phivoc array, depending on which soleng variable we are computing
 *     phiDims:    Vector holding the dimensions of the phi array
 *     natm:        The number of atoms
 *     loc_qt:        The loc_qt array.  This is a [3][8][natm] int array, but must be passed as a pointer,
 *             since natm is a variable.
 *    charget:    charget array.  This is an [8][natm] int array.
 */
void
computeSoleng(double& soleng, Mat<>& phi, Mat<>& charget, Mat<int>& loc_qt) {
    soleng = 0.0;
    for (size_t iind = 1; iind <= charget.nx(); iind++) {
    for (size_t jind = 1; jind <= charget.ny(); jind++) {
        int i = loc_qt(iind,jind,1);
        int j = loc_qt(iind,jind,2);
        int k = loc_qt(iind,jind,3);

        soleng += 0.5*charget(iind,jind)*phi(i,j,k);
    }}
}

double maxrms(double* sumpot, double* expv, double* elec, double gama, int nmol) {
    double rms = 0.0;
    double snum[50]; // constants are f90 legacy
    double err[50];
    for (int i = 0; i < nmol; i++) {
        snum[i] = gama * sumpot[i] + elec[i];
        err[i] = snum[i] - expv[i];
        rms += err[i] * err[i];
    }
    return sqrt(rms / nmol);
}

size_t loadData(std::ifstream& f, //<i
        int imord, //<i
        int ffmodel, //<i
        double radexp, //<i
        double* expv, //<o expv[imord]
        double xyzr[MAXATOMS][XYZRWIDTH], //<o
        double* pqr, //<o
        double* ljepsilon //<o
        ) {
    std::string molecule;
    double val;
    f >> molecule >> val;
    std::cout << "expSolv:\t" << val << "\t" << molecule << std::endl;
    expv[imord] = val;

    std::string atomFileName = molecule + ".xyzr";
    std::ifstream atomFile;
    atomFile.open(atomFileName.c_str());

    size_t natm = 0;
    initXYZR(xyzr);
    std::fill(ljepsilon, ljepsilon+MAXATOMS, 0.0);
    while (atomFile >> xyzr[natm][0]){
        atomFile >> xyzr[natm][1] >> xyzr[natm][2] >> xyzr[natm][3] >> pqr[natm];
        if (ffmodel!=1){ atomFile >> ljepsilon[natm]; }
        atomFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // skip remainder of line
        xyzr[natm][3] *= radexp;
        natm++;
    }
    
    std::cout << natm << std::endl;
    return natm;
}

GeoflowOutput geoflowSolvation(double xyzr[MAXATOMS][XYZRWIDTH], size_t natm, double dcel, int ffmodel, double extvalue, double* pqr, int maxstep, double crevalue, int iadi, double tottf, double* ljepsilon, double alpha, int igfin, double epsilons, double epsilonp, double tol, int iterf, double tpb, int itert, double pres, double gama, double tauval, double prob, int vdwdispersion, double sigmas, double density, double epsilonw) {
    double elec = 0.0, area = 0.0, volume = 0.0, attint = 0.0;

    comdata.dcel = dcel;
    comdata.deltax = comdata.deltay = comdata.deltaz = dcel;
    comdata.pi = acos(-1.0);

    lj.ffmodel = ffmodel;
    lj.tauval = tauval;
    lj.prob = prob;
    lj.vdwdispersion = vdwdispersion;
    lj.sigmas = sigmas;
    lj.density = density;
    lj.epsilonw = epsilonw;
    lj.roro = density / gama;
    double potcoe = 1.0 / gama;
    lj.conms = pres / gama;

    if(ffmodel == 1){
//        std::replace_if(xyzr, numbers+6, bind1st(equal_to<int>(),0) );
        for(size_t i = 0; i<natm; ++i){
            xyzr[i][3] = (xyzr[i][3] < 1e-6) ? 1.21 : xyzr[i][3];
        }
    }

    domainini(xyzr, natm, extvalue);

    Mat<> charget(natm, 8);
    Mat<> corlocqt(natm, 8, 3);
    Mat<int> loc_qt(natm,8,3); 

    for (size_t iatm = 1; iatm <= natm; iatm++) {
        chargedist(xyzr, pqr, charget, corlocqt, loc_qt, iatm);
    }

    int nx = comdata.nx, ny = comdata.ny, nz = comdata.nz;
    comdata.xc.resize(nx);
    for (int i=0; i<nx; i++) { comdata.xc[i] = comdata.xleft + (i-1)*comdata.dcel; }

    comdata.yc.resize(ny);
    for (int i=0; i<ny; i++) { comdata.yc[i] = comdata.yleft + (i-1)*comdata.dcel; }

    comdata.zc.resize(nz);
    for (int i=0; i<nz; i++) { comdata.zc[i] = comdata.zleft + (i-1)*comdata.dcel; }

    vector<double> solv(maxstep+1);
    double diffEnergy = 100;

    int iloop = 0, ipath = 0;
    double tott = 0.0;
std::cout << nx <<" "<<ny<<" "<<nz<<std::endl;
    Mat<> phi(nx,ny,nz), phix(nx,ny,nz), phivoc(nx,ny,nz), surfu(nx,ny,nz), eps(nx,ny,nz), bg(nx,ny,nz);
    while ((iloop < maxstep) && (diffEnergy > crevalue)) {
        iloop++;
        double deltat = 0; //this is wrong for adi...
        if (!iadi) {
            deltat = comdata.dcel * comdata.dcel / 4.5;
        }
        if (ipath == 0) /* will always be true, but this is how
                  * it was in the original F90 code.  
                  * TODO: rewrite this to remove the ipath variable (?) */ {
            tott = fmax(tottf - iloop + 1, 1.0);
        } else {
            if (iloop == 1) {
                tott = tottf;
            } else {
                tott = iloop * comdata.dcel * comdata.dcel / 4.5;
            }
        }
        area = volume = attint = 0.0;
        yhsurface(xyzr, ljepsilon, natm, tott, deltat, phix, surfu, iloop, area, volume, attint, alpha, iadi, igfin);
        normalizeSurfuAndEps(surfu, eps, epsilons, epsilonp);
        if (iloop == 1) {
            seteqb(bg, xyzr, pqr, charget, corlocqt, epsilons);
        }

        int iter = 1000;
        double fpb, titer = 0.0;
        pbsolver(eps, phi, bg, comdata.dcel, tol, iter);
        if (iloop == 1) {
            fpb = titer;
            iterf = iter;
        }
        tpb = tpb + iter;
        itert += iter;

        eps = epsilonp;
        pbsolver(eps, phivoc, bg, comdata.dcel, tol, iter);

        double weit = 1.0 / (2.0 * comdata.dcel);
        for (size_t ix = 2; ix <= comdata.nx - 1; ix++) {
        for (size_t iy = 2; iy <= comdata.ny - 1; iy++) {
        for (size_t iz = 2; iz <= comdata.nz - 1; iz++) {
            double phixx = phi(ix+1,iy,iz) - weit*phi(ix-1,iy,iz);
            double phixy = phi(ix,iy+1,iz) - weit*phi(ix,iy-1,iz);
            double phixz = phi(ix,iy,iz+1) - weit*phi(ix,iy,iz-1);

            phix(ix,iy,iz) = 0.5 * (epsilons - epsilonp) * (phixx * phixx + phixy * phixy + phixz * phixz) * potcoe;
        }}}
        
        // solvation
        double soleng1, soleng2;
        soleng1 = soleng2 = 0.0;
        computeSoleng(soleng1, phi, charget, loc_qt);
        computeSoleng(soleng2, phivoc, charget, loc_qt);
        elec = (soleng1 - soleng2) * 332.0716;
        solv[iloop - 1] = elec + gama * (area + volume * lj.conms + attint * lj.roro);
        if (iloop > 1) {
            diffEnergy = abs((solv[iloop - 1] - solv[iloop - 2]));
        }
        std::cout << solv[iloop-1] << std::endl;
    }



    double sumpot = area + volume * lj.conms + attint * lj.roro;
    double nonpolarSolvation = sumpot*gama;
    double totalSolvation = nonpolarSolvation + elec;

    return (GeoflowOutput) {
        area, volume, attint, sumpot, totalSolvation, nonpolarSolvation, elec
    };
}

void
processAtomsFile(std::string fileName, int ffmodel, int radexp, double extvalue, int maxstep, double crevalue, int iadi, double tottf, double alpha, int igfin, double epsilons, double epsilonp, double tol, double &tpb, int &iterf, int &itert, double pres, double gama, double dcel, double tauval, double prob, int vdwdispersion, double sigmas, double density, double epsilonw) {
    std::ifstream f;
    f.open(fileName.c_str());

    int nmol = numberOfLines(fileName);
    for (int imord = 0; imord < nmol; imord++) {
        double xyzr[MAXATOMS][XYZRWIDTH];
        double ljepsilon[MAXATOMS];
        double pqr[MAXATOMS];
        double expv[100]; // hardcoded 100 constant, as per interface to F90 code :(

        size_t natm = loadData(f, imord, ffmodel, radexp, expv, xyzr, pqr, ljepsilon);

        GeoflowOutput gf = geoflowSolvation(xyzr, natm, dcel, ffmodel, extvalue, pqr, maxstep, crevalue, iadi, tottf, ljepsilon, alpha, igfin, epsilons, epsilonp, tol, iterf, tpb, itert, pres, gama, tauval, prob, vdwdispersion, sigmas, density, epsilonw);

        std::cout << "totalSolv:\t" << gf.totalSolvation << "\t";
        std::cout << "nonpolar: " << gf.nonpolarSolvation << "\t";
        std::cout << "electro: " << gf.elecSolvation << "\n" << std::endl;
    }
    //    std::cout << "RMS Error: " << maxrms( (double*)sumpot, (double*)expv, (double*)elec, gama, nmol) << std::endl;
}

void
setPresStep(double &pres_step, double pres) {
    if (pres < 0.001) {
        pres_step = 0.0001;
    } else if ((pres > 0.001) && (pres < 0.01)) {
        pres_step = 0.001;
    } else // if( pres >= 0.001)
    {
        pres_step = 0.005;
    }
}

void
setGamaStep(double &gama_step, double gama) {
    if ((gama >= 0.00001) && (gama < 0.0001)) {
        gama_step = 0.00001;
    } else if ((gama >= 0.0001) && (gama < 0.001)) {
        gama_step = 0.0001;
    } else if ((gama >= 0.001) && (gama < 0.01)) {
        gama_step = 0.001;
    } else if ((gama > 0.01) && (gama <= 0.055)) {
        gama_step = 0.005;
    } else // gama > 0.055
    {
        gama_step = 0.005;
    }
}

void
pbconcz2(
        // These parameters correspond directly to those read in via the datafiles (fort.12 and 17set.txt)
        // in the original Fortran code    
 //       int nmol,
        double pres_i,
        double gama_i,
        int npiter,
        int ngiter,
        double tauval,
        double prob,
        int ffmodel, // 1 for ZAP-9/AM1-BCCv1; 2 for OPLS/AA
        double sigmas, // Angstrom (radius of water molecule based on LJ parameter sigma)
        double epsilonw, // epsilon parameter of 0 (kcal/mol) of water molecule
        int vdwdispersion, // 1(on) or 0(off)- previously called REPULSIVE
        double extvalue, // (distance atom surface and box boundary)
        //    int    iprec,    // flag to indicate the usage of preconditioner iprec =1 (yes); 0 (no)
        //    int    istep,
        int iadi, // 0 for explicit scheme; 1 for ADI scheme
        double alpha, //  weight of previous solution to change the next solution in geometry flow
        //    int        ipbin, //  start guess for PB 1; inherit '0'
        double tol,
        double tottf, //  total time
        double dcel,
        int maxstep,
        double epsilons,
        double epsilonp,
        int radexp,
        double crevalue,
  //      int idacsl, //  0 for solvation force calculation; 1 or accuracy test
        double density //  (use 0.03346) 
        ) {
    std::cout << ((ffmodel==1)?"ZAP":"OPLS") << std::endl;
    double pres = pres_i;
    //    writeSupportingFiles(nmol);

    double pres_step;
    for (int indpres = 1; indpres <= npiter; indpres++) {
        setPresStep(pres_step, pres);
        if (indpres > 1) {
            pres += pres_step;
        }

        double gama = gama_i,
                gama_step;
        for (int indgama = 1; indgama <= ngiter; indgama++) {
            setGamaStep(gama_step, gama);
            std::cout << "GAMA_STEP= " << gama_step << std::endl;
            if (indgama > 1) {
                gama += gama_step;
            }
        }

        std::cout << "GAMA = " << gama << std::endl;
        //        std::cout << "RORO = " << lj.roro << std::endl;
        //        std::cout << "POTCOE = " << potcoe << std::endl;
        std::cout << "PRES = " << pres << std::endl;
        //        std::cout << "CONMS = " << lj.conms << std::endl;

        int igfin = 1;
        double tpb = 0.0;
        int iterf = 0, itert = 0;

        processAtomsFile("molecules.txt", ffmodel, radexp, extvalue, maxstep, crevalue, iadi, tottf, alpha, igfin, epsilons, epsilonp, tol, tpb, iterf, itert, pres, gama, dcel, tauval, prob, vdwdispersion, sigmas, density, epsilonw);
    }
}

