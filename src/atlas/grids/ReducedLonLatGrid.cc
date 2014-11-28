/*
 * (C) Copyright 1996-2014 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <typeinfo> // std::bad_cast
#include <eckit/memory/Builder.h>

#include "atlas/atlas_config.h"
#include "atlas/grids/ReducedLonLatGrid.h"
#include "atlas/GridSpec.h"
#include "atlas/util/Debug.h"

using eckit::Params;
using eckit::BadParameter;

namespace atlas {
namespace grids {

std::string ReducedLonLatGrid::gtype()
{
  return "reduced_ll";
}

std::string ReducedLonLatGrid::className()
{
  return "atlas.grid.ReducedLonLatGrid";
}

void ReducedLonLatGrid::set_typeinfo()
{
  std::stringstream stream;
  stream << gtype()<<".N"<<N();
  uid_ = stream.str();
  hash_ = stream.str();
  grid_type_ = gtype();
}

ReducedLonLatGrid::ReducedLonLatGrid() : ReducedGrid()
{
}

ReducedLonLatGrid::ReducedLonLatGrid( const int nlat, const int nlons[], bool pole )
{
  ReducedGrid::N_ = nlat;
  pole_ = pole;
  setup(nlat,nlons,pole_);
  set_typeinfo();
}

ReducedLonLatGrid::ReducedLonLatGrid( const Params& params )
{
  setup(params);
  mask(params);
  set_typeinfo();
}

void ReducedLonLatGrid::setup( const Params& params )
{
  if( ! params.has("nlat") )         throw BadParameter("N missing in Params",Here());
  if( ! params.has("npts_per_lat") ) throw BadParameter("npts_per_lat missing in Params",Here());

  int nlat = params["nlat"];

  if( params.has("N") )
    N_ = params["N"];
  else
    N_ = nlat;

  pole_ = false;
  if( params.has("pole") ) pole_ = params["pole"];


  eckit::ValueList list = params.get("npts_per_lat");
  std::vector<int> nlons(list.size());
  for(int j=0; j<nlons.size(); ++j)
    nlons[j] = list[j];

  if( params.has("latitudes") )
  {
    ReducedGrid::setup(params);
  }
  else
  {
    setup(nlat,nlons.data(),pole_);
  }
}

void ReducedLonLatGrid::setup( const int nlat, const int nlons[], bool pole )
{
  std::vector<double> lats (nlat);

  double delta, latmax;

  if( pole )
  {
    delta = 180./static_cast<double>(nlat-1);
    latmax = 90.;
  }
  else
  {
    delta = 180./static_cast<double>(nlat);
    latmax = 90. - 0.5*delta;
  }

  for( int jlat=0; jlat<nlat; ++jlat )
  {
    lats[jlat] = latmax - static_cast<double>(jlat)*delta;
  }
  ReducedGrid::setup(lats.size(),lats.data(),nlons);

  eckit::Log::info() << "nlat = " << ReducedLonLatGrid::nlat() << std::endl;
  eckit::Log::info() << "latmin = " << lat(ReducedLonLatGrid::nlat()-1) << std::endl;
  eckit::Log::info() << "latmax = " << lat(0) << std::endl;

//  ReducedGrid::setup_lat_hemisphere(N,lats.data(),nlons,DEG);
}


GridSpec ReducedLonLatGrid::spec() const
{
  GridSpec grid_spec( gtype() );

  grid_spec.uid(uid());
  grid_spec.set("hash", hash() );

  grid_spec.set("N", N() );
  grid_spec.set("nlat", nlat() );
  grid_spec.set_npts_per_lat(npts_per_lat());

  if( nlat() != N() )
    grid_spec.set_latitudes(latitudes());

  grid_spec.set("pole",pole_);
  if( !bounding_box().global() )
    grid_spec.set_bounding_box(bounding_box());

  return grid_spec;
}

} // namespace grids
} // namespace atlas
