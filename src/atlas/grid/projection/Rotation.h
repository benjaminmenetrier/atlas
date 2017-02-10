#pragma once

#include "eckit/geometry/KPoint.h"
#include "eckit/geometry/Point2.h"
#include "eckit/config/Parametrisation.h"
#include "eckit/value/Properties.h"

namespace atlas {
namespace grid {
namespace projection {

class Rotated {

public:

    Rotated( const eckit::Parametrisation& );
    Rotated( const Rotated& rhs ); // copy constructor
    virtual ~Rotated() {} // destructor should be virtual when using a virtual copy constructor
    static std::string classNamePrefix() { return "Rotated"; }
    static std::string typePrefix() { return "rotated_"; }

    void rotate(eckit::geometry::LLPoint2 &P) const;    // coordinates of the point on a rotated sphere with specified pole
    void unrotate(eckit::geometry::LLPoint2 &P) const;    // inverse operation of rotate

    void spec(eckit::Properties&) const;

private:

    eckit::geometry::LLPoint2 pole_;    // pole
    double cos_latrp_; //  cos( 90 - pole_lat )
    double sin_latrp_; //  sin( 90 - pole_lat )

};

class NotRotated {

public:

    NotRotated( const eckit::Parametrisation& ) {}
    NotRotated( const NotRotated& ) {} // copy constructor
    virtual ~NotRotated() {} // destructor should be virtual when using a virtual copy constructor
    static std::string classNamePrefix() { return ""; } // deliberately empty
    static std::string typePrefix() { return ""; }      // deliberately empty

    void rotate(eckit::geometry::LLPoint2&) const { /* do nothing */ }
    void unrotate(eckit::geometry::LLPoint2&) const { /* do nothing */ }

    void spec(eckit::Properties&) const {}
};

}  // namespace projection
}  // namespace grid
}  // namespace atlas
