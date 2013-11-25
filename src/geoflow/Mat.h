///  @file    Mat.h
///  @author  Andrew Stevens
///  @brief Wrapper class for 2d and 3d arrays
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

#ifndef MAT_H
#define MAT_H

#include <valarray>
#include <vector>
#include <cmath>

template<typename T> class Stencil;

template<typename T=double> struct Mat;
template<typename T> struct Mat{
    friend class Stencil<T>;

    size_t _nx,_ny,_nz;
    size_t nx()const{return _nx;}
    size_t ny()const{return _ny;}
    size_t nz()const{return _nz;}

    T* _data;
    std::vector<T>* vec;
    Mat(T* data, size_t nx, size_t ny, size_t nz=1):  _nx(nx), _ny(ny), _nz(nz), _data(data), vec(0) {} //this needs to die
    Mat(std::vector<T>& val, size_t nx, size_t ny, size_t nz=1): _nx(nx), _ny(ny), _nz(nz), _data(val.data()), vec(&val) {}

    ~Mat(){};

    bool operator==(const Mat<T>& rhs) const { return (this->_data == rhs._data) && (this->_nx == rhs._nx) && (this->_ny == rhs._ny) && (this->_nz == rhs._nz); }

    size_t index(size_t x, size_t y, size_t z) const { return x-1 + _nx*((y-1) + _ny*(z-1)); }
    T& operator()(size_t x, size_t y, size_t z=1){ return _data[index(x,y,z)]; }
    T operator()(size_t x, size_t y, size_t z=1) const { return _data[index(x,y,z)]; }
    
    T& operator[](size_t i){ return _data[i]; }
    T operator[](size_t i) const { return _data[i]; }

    T* data(){return _data;}
    size_t size(){return _nx*_ny*_nz;}
    T* end(){return _data + size();}

    Mat<T>& operator=(T a){ std::fill(data(), end(), a); return *this; }
    Mat<T>& operator=(Mat a){
//        if(vec==0){
            std::copy(a.data(), a.end(), data()); return *this;
//        }else{
//            vec->swap(*(a.vec)); _data = vec->data(); return *this;
//        }

    }

    Stencil<T> stencilBegin(T h) { return Stencil<T>(*this, h, 2,2,2); }
    Stencil<T> stencilEnd(T h) { return Stencil<T>(*this, h, _nx-1,_ny-1,_nz-1); }
};

template<typename T> struct Stencil: public std::iterator<std::forward_iterator_tag, T>
{//this should be a const iterator :)
    Mat<T>& _mat;
    const T h,halfh,h2,qrth2;
    size_t i;
    const  size_t yStep, zStep;
 
    T *c;// *xm,*ym,*zm, *xp,*yp,*zp,
    //  *xym,*xzm,*yzm, *xyp,*xzp,*yzp,
    //  *xm_yp,*xm_zp,*ym_zp, *xp_ym,*xp_zm,*yp_zm;

    //Stencil(Stencil<T>&) default
    //Stencil() default
    //Stencil& operator=() default

    Stencil(Mat<T>& mat, T _h, size_t x,size_t y,size_t z): _mat(mat), h(1.0/_h), halfh(1.0/(2*_h)), h2(1./(_h*_h)),
        qrth2(1.0/(4*_h*_h)), i(_mat.index(x,y,z)), yStep(_mat.nx()), zStep(_mat.nx()*_mat.ny()),
        c(_mat.data() + i)
    //    xm(_mat.data() + _mat.index(x-1,y,z)), ym(_mat.data() + _mat.index(x,y-1,z)), zm(_mat.data() + _mat.index(x,y,z-1)),
    //    xp(_mat.data() + _mat.index(x+1,y,z)), yp(_mat.data() + _mat.index(x,y+1,z)), zp(_mat.data() + _mat.index(x,y,z+1)),
    //    xym(_mat.data() + _mat.index(x-1,y-1,z)), xzm(_mat.data() + _mat.index(x-1,y,z-1)), yzm(_mat.data() + _mat.index(x,y-1,z-1)),
    //    xyp(_mat.data() + _mat.index(x+1,y+1,z)), xzp(_mat.data() + _mat.index(x+1,y,z+1)), yzp(_mat.data() + _mat.index(x,y+1,z+1)),
    //    xm_yp(_mat.data() + _mat.index(x-1,y+1,z)), xm_zp(_mat.data() + _mat.index(x-1,y,z+1)), ym_zp(_mat.data() + _mat.index(x,y-1,z+1)),
    //    xp_ym(_mat.data() + _mat.index(x+1,y-1,z)), xp_zm(_mat.data() + _mat.index(x+1,y,z-1)), yp_zm(_mat.data() + _mat.index(x,y+1,z-1))
    {}

    bool operator==(const Stencil<T>& rhs) const { return (this->_mat == rhs._mat) && (this->c == rhs.c); }
    bool operator!=(const Stencil<T>& rhs) const { return !(*this == rhs); }

    T& operator*() { return *c; }
    const T* operator->() { return c; } //???
    Stencil& operator++() { next(); return *this; }
    Stencil operator++ ( int ){
          Stencil<T> clone( *this );
          operator++();
          return clone;
    }

    void next(){
        ptrStep(1);
        if((i+1) % yStep == 0){
            if( (i+1+yStep) % (zStep) == 0 ){
                ptrStep(yStep + 2);
            }else{
                ptrStep(2);
            }
        }
    }

    void ptrStep(size_t numSteps){
        i += numSteps;
        c += numSteps;
  //      xm += numSteps; ym += numSteps; zm += numSteps;
  //      xp += numSteps; yp += numSteps; zp += numSteps;
  //      xym += numSteps; xzm += numSteps; yzm += numSteps;
  //      xyp += numSteps; xzp += numSteps; yzp += numSteps;
  //      xm_yp += numSteps; xm_zp += numSteps; ym_zp += numSteps;
  //      xp_ym += numSteps; xp_zm += numSteps; yp_zm += numSteps;
    }

    const T* xm() const { return c-1; } 
    const T* xp() const { return c+1; } 
    const T* ym() const { return c-yStep; } 
    const T* yp() const { return c+yStep; } 
    const T* zm() const { return c-zStep; } 
    const T* zp() const { return c+zStep; } 

    const T* xym() const { return xm() - yStep; } 
    const T* xyp() const { return xp() + yStep; } 
    const T* xzm() const { return xm() - zStep; } 
    const T* xzp() const { return xp() + zStep; } 
    const T* yzm() const { return zm() - yStep; } 
    const T* yzp() const { return zp() + yStep; } 

    const T* xm_yp() const { return xm() + yStep; } 
    const T* xp_ym() const { return xp() - yStep; } 
    const T* xm_zp() const { return xm() + zStep; } 
    const T* xp_zm() const { return xp() - zStep; } 
    const T* ym_zp() const { return zp() - yStep; } 
    const T* yp_zm() const { return zm() + yStep; } 

    T dx() const { return halfh*(*xp() - *xm()); }
    T dy() const { return halfh*(*yp() - *ym()); }
    T dz() const { return halfh*(*zp() - *zm()); }

    T dxx() const { return h2*(*xp() - 2.0*(*c) + *xm()); }
    T dyy() const { return h2*(*yp() - 2.0*(*c) + *ym()); }
    T dzz() const { return h2*(*zp() - 2.0*(*c) + *zm()); }

    T dxy() const { return qrth2*(*xyp() + *xym() - *xm_yp() - *xp_ym()); }
    T dxz() const { return qrth2*(*xzp() + *xzm() - *xm_zp() - *xp_zm()); }
    T dyz() const { return qrth2*(*yzp() + *yzm() - *ym_zp() - *yp_zm()); }

    T deriv(T tx) const {
        const double dphi =
            (1.0 + dx()*dx() + dy()*dy())*dzz()
          + (1.0 + dx()*dx() + dz()*dz())*dyy()
          + (1.0 + dy()*dy() + dz()*dz())*dxx()
          - 2.0*( dx()*dy()*dxy() + dx()*dz()*dxz() + dy()*dz()*dyz() );

        const double gram = 1.0 + dx()*dx() + dy()*dy() + dz()*dz();
        
        return dphi/gram + sqrt(gram)*tx;//(x,y,z);
    }

};


#endif

