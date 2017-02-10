#pragma once

#include "atlas/grid/projection/Projection.h"
#include "atlas/grid/projection/Rotation.h"

namespace atlas {
namespace grid {
namespace projection {

template <typename Rotation>
class MercatorProjectionT: public Projection {

public:

    // constructor
    MercatorProjectionT(const eckit::Parametrisation& p);

    // copy constructor
    MercatorProjectionT( const MercatorProjectionT& rhs );

    // clone method
    virtual Projection* clone() const ;

    // class name
    static std::string className() { return "atlas."+Rotation::classNamePrefix()+"MercatorProjection"; }
    static std::string projection_type_str() { return Rotation::typePrefix()+"mercator"; }
    virtual std::string virtual_projection_type_str() const { return Rotation::typePrefix()+"mercator"; }

    // projection and inverse projection
    eckit::geometry::LLPoint2 coords2lonlat(eckit::geometry::Point2)   const;
    eckit::geometry::Point2   lonlat2coords(eckit::geometry::LLPoint2) const;

    // TODO: the comment says lambert, but this is Mercator.... ?
    bool isRegional() { return true; }  // lambert projection cannot be used for global grids

    // specification
    virtual eckit::Properties spec() const;

protected:

    double lon0_;            // central longitude
    double radius_;          // sphere radius

    void setup(const eckit::Parametrisation& p);

private:

  Rotation rotation_;

};

typedef MercatorProjectionT<NotRotated> MercatorProjection;
typedef MercatorProjectionT<Rotated>    RotatedMercatorProjection;


}  // namespace projection
}  // namespace grid
}  // namespace atlas
