/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "atlas/trans/local/VorDivToUVLocal.h"

#include "atlas/functionspace/Spectral.h"
#include "atlas/runtime/Log.h"
#include "atlas/util/Earth.h"

using atlas::functionspace::Spectral;
using atlas::FunctionSpace;

namespace atlas {
namespace trans {

namespace {
static VorDivToUVBuilder<VorDivToUVLocal> builder("local");
}

// --------------------------------------------------------------------------------------------------------------------
// Routine to copy spectral data into internal storage form of IFS trans
// Ported to C++ by: Andreas Mueller *ECMWF*
void prfi1b(
    const int truncation,
    const int km, // zonal wavenumber
    const int nb_fields, // number of fields
    const double rspec[], // spectral data
    double pia[]) // spectral components in data layout of trans library
{
    int ilcm = truncation+1-km, ioff = (2*truncation-km+3)*km, nlei1 = truncation+4+(truncation+4+1)%2;
    for( int j=1; j<=ilcm; j++ ) {
        int inm = ioff+(ilcm-j)*2;
        for( int jfld=0; jfld<nb_fields; jfld++ ) {
            int ir = 2*jfld, ii = ir+1;
            pia[ir*nlei1+j+1] = rspec[inm*nb_fields+jfld];
            pia[ii*nlei1+j+1] = rspec[(inm+1)*nb_fields+jfld];
        }
    }

    for( int jfld=0; jfld<2*nb_fields; jfld++ ) {
        pia[jfld*nlei1  ]      = 0.;
        pia[jfld*nlei1+1]      = 0.;
        pia[jfld*nlei1+ilcm+2] = 0.;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Routine to compute spectral velocities (*cos(latitude)) out of spectral vorticity and divergence
// Reference:
//        ECMWF Research Department documentation of the IFS
//        Temperton, 1991, MWR 119 p1303
// Ported to C++ by: Andreas Mueller *ECMWF*
void vd2uv(
    const int truncation,
    const int km, // zonal wavenumber
    const int nb_vordiv_fields,
    const double vorticity_spectra[],
    const double divergence_spectra[],
    double U[],
    double V[],
    const eckit::Configuration& config )
{

    int nlei1 = truncation+4+(truncation+4+1)%2;

    // repsnm: epsilon from eq.(2.12) and (2.13) in [Temperton 1991]
    std::vector<double> repsnm((truncation+1)*(truncation+6)/2);
    int idx = 0;
    for( int jm=0; jm<=truncation; ++jm ) {
        for( int jn=jm; jn<=truncation+2; ++jn, ++idx ) {
            repsnm[idx] = std::sqrt((jn*jn-jm*jm)/(4.*jn*jn-1.));
        }
    }
    repsnm[0] = 0.;

    // rlapin: constant factor from eq.(2.2) and (2.3) in [Temperton 1991]
    std::vector<double> rlapin(truncation+3);
    for( int jn=1; jn<=truncation+2; ++jn ) {
        rlapin[jn] = -util::Earth::radiusInMeters()*util::Earth::radiusInMeters()/(jn*(jn+1.));
    }
    rlapin[0] = 0.;

    // inverse the order of repsnm and rlapin for improved accuracy
    std::vector<double> zepsnm(truncation+6);
    std::vector<double> zlapin(truncation+6);
    std::vector<double> zn    (truncation+6);
    for( int jn=km-1; jn<=truncation+2; ++jn ) {
        int ij = truncation+3-jn;
        if( jn>=0 ) {
            zlapin[ij] = rlapin[jn];
            if ( jn<km ) {
                zepsnm[ij] = 0.;
            } else {
                zepsnm[ij] = repsnm[jn+(2*truncation-km+5)*km/2];
            }
        } else {
            zlapin[ij] = 0.;
            zepsnm[ij] = 0.;
        }
        zn[ij]     = jn;
    }
    zn[0] = truncation+3;

    // copy spectral data into internal trans storage:
    std::vector<double> rvor(2*nb_vordiv_fields*nlei1);
    std::vector<double> rdiv(2*nb_vordiv_fields*nlei1);
    std::vector<double> ru  (2*nb_vordiv_fields*nlei1);
    std::vector<double> rv  (2*nb_vordiv_fields*nlei1);
    prfi1b(truncation, km, nb_vordiv_fields, vorticity_spectra,  rvor.data());
    prfi1b(truncation, km, nb_vordiv_fields, divergence_spectra, rdiv.data());

    // compute eq.(2.12) and (2.13) in [Temperton 1991]:
    if( km==0 ) {
        for( int jfld=0; jfld<nb_vordiv_fields; ++jfld ) {
            int ir=2*jfld*nlei1-1;
            for( int ji=2; ji<truncation+4-km; ++ji ) {
                ru[ir+ji] = + zn[ji+1]*zepsnm[ji  ]*zlapin[ji+1]*rvor[ir+ji+1]
                            - zn[ji-2]*zepsnm[ji-1]*zlapin[ji-1]*rvor[ir+ji-1];
                rv[ir+ji] = - zn[ji+1]*zepsnm[ji  ]*zlapin[ji+1]*rdiv[ir+ji+1]
                            + zn[ji-2]*zepsnm[ji-1]*zlapin[ji-1]*rdiv[ir+ji-1];
            }
        }
    } else {
        for( int jfld=0; jfld<nb_vordiv_fields; ++jfld) {
            int ir=2*jfld*nlei1-1, ii = ir+nlei1;
            for( int ji=2; ji<truncation+4-km; ++ji ) {
                ru[ir+ji] = -                    km*zlapin[ji  ]*rdiv[ii+ji  ]
                            + zn[ji+1]*zepsnm[ji  ]*zlapin[ji+1]*rvor[ir+ji+1]
                            - zn[ji-2]*zepsnm[ji-1]*zlapin[ji-1]*rvor[ir+ji-1];
                ru[ii+ji] = +                    km*zlapin[ji  ]*rdiv[ir+ji  ]
                            + zn[ji+1]*zepsnm[ji  ]*zlapin[ji+1]*rvor[ii+ji+1]
                            - zn[ji-2]*zepsnm[ji-1]*zlapin[ji-1]*rvor[ii+ji-1];
                rv[ir+ji] = -                    km*zlapin[ji  ]*rvor[ii+ji  ]
                            - zn[ji+1]*zepsnm[ji  ]*zlapin[ji+1]*rdiv[ir+ji+1]
                            + zn[ji-2]*zepsnm[ji-1]*zlapin[ji-1]*rdiv[ir+ji-1];
                rv[ii+ji] = +                    km*zlapin[ji  ]*rvor[ir+ji  ]
                            - zn[ji+1]*zepsnm[ji  ]*zlapin[ji+1]*rdiv[ii+ji+1]
                            + zn[ji-2]*zepsnm[ji-1]*zlapin[ji-1]*rdiv[ii+ji-1];
            }
        }
    }

    // copy data from internal storage back to external spectral data:
    int ilcm = truncation-km;
    int ioff = (2*truncation-km+3)*km;
    // ioff: start index of zonal wavenumber km in spectral data
    double za_r = 1./util::Earth::radiusInMeters();
    for( int j=0; j<=ilcm; ++j ) {
        // ilcm-j = total wavenumber
        int inm = ioff+(ilcm-j)*2;
        for( int jfld=0; jfld<nb_vordiv_fields; ++jfld ) {
            int ir = 2*jfld*nlei1, ii = ir + nlei1;
            int idx = inm*nb_vordiv_fields+jfld;
            // real part:
            U[idx] = ru[ir+j+2]*za_r;
            V[idx] = rv[ir+j+2]*za_r;
            idx += nb_vordiv_fields;
            // imaginary part:
            U[idx] = ru[ii+j+2]*za_r;
            V[idx] = rv[ii+j+2]*za_r;
        }
    }

}

void VorDivToUVLocal::execute( const int nb_coeff, const int nb_fields,
                               const double vorticity[], const double divergence[],
                               double U[], double V[],
                               const eckit::Configuration& config ) const
{
    for( int jm=0; jm<=truncation_; ++jm ) {
        vd2uv(truncation_, jm, nb_fields, vorticity, divergence, U, V, config);
    }

}


VorDivToUVLocal::VorDivToUVLocal( const int truncation, const eckit::Configuration& config ) :
  truncation_(truncation) {
}

VorDivToUVLocal::VorDivToUVLocal( const FunctionSpace& fs, const eckit::Configuration& config ) :
  truncation_( Spectral(fs).truncation() ) {
}


VorDivToUVLocal::~VorDivToUVLocal()
{
}

} // namespace trans
} // namespace atlas
