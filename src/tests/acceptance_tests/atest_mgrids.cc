/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <vector>

#include "eckit/log/OStreamTarget.h"

#include "atlas/field.h"
#include "atlas/functionspace.h"
#include "atlas/grid.h"
#include "atlas/interpolation/Interpolation.h"
#include "atlas/mesh.h"
#include "atlas/mesh/actions/BuildHalo.h"
#include "atlas/meshgenerator.h"
#include "atlas/numerics/fvm/Method.h"
#include "atlas/option.h"
#include "atlas/output/Gmsh.h"
#include "atlas/parallel/mpi/mpi.h"
#include "atlas/runtime/AtlasTool.h"
#include "atlas/runtime/Log.h"
#include "atlas/util/Config.h"
#include "atlas/util/Constants.h"
#include "atlas/util/CoordinateEnums.h"

using namespace atlas;

double vortex_rollup( double lon, double lat, double t, double mean );

//------------------------------------------------------------------------------

class Program : public AtlasTool {
    virtual int execute( const Args& args );

public:
    Program( int argc, char** argv );
};

//-----------------------------------------------------------------------------

Program::Program( int argc, char** argv ) : AtlasTool( argc, argv ) {
    add_option( new SimpleOption<std::string>( "gridA", "grid A" ) );
    add_option( new SimpleOption<std::string>( "gridB", "grid B" ) );
    add_option( new SimpleOption<bool>( "ghost", "Output ghost elements" ) );
    add_option( new SimpleOption<long>( "haloA", "Halo size for A" ) );
    add_option( new SimpleOption<long>( "haloB", "Halo size for B" ) );
    add_option( new SimpleOption<bool>( "matrix-free", "Interpolation without matrix" ) );
    add_option( new SimpleOption<std::string>( "checksum", "File to write that will contains checksums of run" ) );
    add_option( new SimpleOption<bool>( "gmsh", "Output gmsh" ) );
    add_option( new SimpleOption<bool>( "no-validate", "Avoid validation of results to increase runtime" ) );
}

//-----------------------------------------------------------------------------

int Program::execute( const Args& args ) {
    auto matrix_free       = util::Config( "matrix_free", args.getBool( "matrix-free", false ) );
    auto ghost             = util::Config( "ghost", args.getBool( "ghost", false ) );
    auto haloA             = option::halo( args.getLong( "haloA", 2 ) );
    auto haloB             = option::halo( args.getLong( "haloB", 2 ) );
    auto checksum_filepath = args.getString( "checksum", displayName() + ".checksum" );

    auto gridA = Grid( args.getString( "gridA" ) );
    auto gridB = Grid( args.getString( "gridB" ) );

    auto meshgenerator = MeshGenerator( "structured" );

    Trace setup_A( Here(), "Setup A" );
    auto distA = grid::Distribution( gridA, grid::Partitioner( "trans" ) );
    auto meshA = meshgenerator.generate( gridA, distA );
    functionspace::StructuredColumns fsA( gridA, distA, haloA );
    setup_A.stop();

    Trace setup_B( Here(), "Setup B" );
    auto distB = grid::Distribution( gridB, grid::MatchingMeshPartitioner( meshA ) );
    functionspace::StructuredColumns fsB( gridB, distB, haloB );
    setup_B.stop();

    Trace setup_interpolation_AtoB( Here(), "Setup interpolation AtoB" );
    Interpolation AtoB( option::type( "bicubic" ) | matrix_free, fsA, fsB );
    setup_interpolation_AtoB.stop();

    Trace setup_interpolation_BtoA( Here(), "Setup interpolation BtoA" );
    Interpolation BtoA( option::type( "bicubic" ) | matrix_free, fsB, fsA );
    setup_interpolation_BtoA.stop();

    Field fieldA = fsA.createField<double>( option::name( "fieldA" ) );
    Field fieldB = fsB.createField<double>( option::name( "fieldB" ) );

    auto lonlat = array::make_view<double, 2>( fsA.xy() );
    auto A      = array::make_view<double, 1>( fieldA );
    auto B      = array::make_view<double, 1>( fieldB );

    double meanA = 1.;
    for ( idx_t n = 0; n < fsA.size(); ++n ) {
        A( n ) = vortex_rollup( lonlat( n, LON ), lonlat( n, LAT ), 1., meanA );
    }
    fieldA.set_dirty();
    fieldA.haloExchange();


    Log::Channel checksums;
    checksums.addStream( Log::info() );
    if ( mpi::comm().rank() == 0 ) { checksums.addFile( checksum_filepath ); }

    checksums << "Field A checksum before interpolation AtoB: " << fsA.checksum( fieldA ) << std::endl;

    Trace interpolation_AtoB( Here(), "Interpolation AtoB" );
    AtoB.execute( fieldA, fieldB );
    interpolation_AtoB.stop();

    checksums << "Field B checksum after  interpolation AtoB: " << fsB.checksum( fieldB ) << std::endl;

    for ( idx_t n = 0; n < fsB.size(); ++n ) {
        B( n ) = meanA - B( n );
    }
    fieldB.set_dirty();
    fieldB.haloExchange();

    Trace interpolation_BtoA( Here(), "Interpolation BtoA" );
    BtoA.execute( fieldB, fieldA );
    interpolation_BtoA.stop();

    checksums << "Field A checksum after  interpolation BtoA: " << fsA.checksum( fieldA ) << std::endl;

    Log::info() << "Timers:" << std::endl;
    Log::info() << "  Setup A                    " << setup_A.elapsed() << " s" << std::endl;
    Log::info() << "  Setup B                    " << setup_B.elapsed() << " s" << std::endl;
    Log::info() << "  -----------------------------------------" << std::endl;
    Log::info() << "  Interpolation AtoB Setup   " << setup_interpolation_AtoB.elapsed() << " s" << std::endl;
    Log::info() << "  Interpolation BtoA Setup   " << setup_interpolation_BtoA.elapsed() << " s" << std::endl;
    Log::info() << "  Interpolation AtoB Execute " << interpolation_AtoB.elapsed() << " s" << std::endl;
    Log::info() << "  Interpolation BtoA Execute " << interpolation_BtoA.elapsed() << " s" << std::endl;
    Log::info() << "  -----------------------------------------" << std::endl;
    Log::info() << "  Interpolation AtoB Total   " << setup_interpolation_AtoB.elapsed() + interpolation_AtoB.elapsed()
                << " s" << std::endl;
    Log::info() << "  Interpolation BtoA Total   " << setup_interpolation_BtoA.elapsed() + interpolation_BtoA.elapsed()
                << " s" << std::endl;

    int status = success();

    Mesh meshB;
    if ( not args.getBool( "no-validate", false ) ) {
        double tolerance = 1.e-12;

        meshB               = meshgenerator.generate( gridB, distB );
        auto norm_computerB = functionspace::NodeColumns( meshB );
        double sumB;
        idx_t countB;
        norm_computerB.sum( fieldB, sumB, countB );
        if ( std::abs( sumB ) < tolerance ) {
            Log::info() << "Validation B correct: " << std::abs( sumB ) << " < tolerance [" << tolerance << "]"
                        << std::endl;
        }
        else {
            Log::error() << "Validation B failed: " << std::abs( sumB ) << " > tolerance [" << tolerance << "]"
                         << std::endl;
            status = failed();
        }

        auto norm_computerA = functionspace::NodeColumns( meshA );
        double sumA;
        idx_t countA;
        norm_computerA.sum( fieldA, sumA, countA );
        if ( std::abs( sumA ) < tolerance ) {
            Log::info() << "Validation A correct: " << std::abs( sumA ) << " < tolerance [" << tolerance << "]"
                        << std::endl;
        }
        else {
            Log::error() << "Validation A failed: " << std::abs( sumA ) << " > tolerance [" << tolerance << "]"
                         << std::endl;
            status = failed();
        }
    }

    // Gmsh output
    if ( args.getBool( "gmsh", false ) ) {
        if ( not meshB ) { meshB = meshgenerator.generate( gridB, distB ); }

        auto gmshA = output::Gmsh( displayName() + "_meshA.msh", ghost );
        gmshA.write( meshA );
        gmshA.write( fieldA );

        auto gmshB = output::Gmsh( displayName() + "_meshB.msh", ghost );
        gmshB.write( meshB );
        gmshB.write( fieldB );
    }
    return status;
}

//------------------------------------------------------------------------------

double vortex_rollup( double lon, double lat, double t, double mean ) {
    // lon and lat in degrees!

    // Formula found in "A Lagrangian Particle Method with Remeshing for Tracer Transport on the Sphere"
    // by Peter Bosler, James Kent, Robert Krasny, CHristiane Jablonowski, JCP 2015

    lon *= util::Constants::degreesToRadians();
    lat *= util::Constants::degreesToRadians();

    auto sqr           = []( const double x ) { return x * x; };
    auto sech          = []( const double x ) { return 1. / std::cosh( x ); };
    const double T     = 1.;
    const double Omega = 2. * M_PI / T;
    t *= T;
    const double lambda_prime = std::atan2( -std::cos( lon - Omega * t ), std::tan( lat ) );
    const double rho          = 3. * std::sqrt( 1. - sqr( std::cos( lat ) ) * sqr( std::sin( lon - Omega * t ) ) );
    double omega              = 0.;
    double a                  = util::Earth::radius();
    if ( rho != 0. ) { omega = 0.5 * 3 * std::sqrt( 3 ) * a * Omega * sqr( sech( rho ) ) * std::tanh( rho ) / rho; }
    double q = mean - std::tanh( 0.2 * rho * std::sin( lambda_prime - omega / a * t ) );
    return q;
};

//------------------------------------------------------------------------------

int main( int argc, char** argv ) {
    Program tool( argc, argv );
    return tool.start();
}
