/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


#pragma once

#include <type_traits>

#include "eckit/config/Parametrisation.h"
#include "eckit/linalg/SparseMatrix.h"

#include "atlas/array.h"
#include "atlas/field/Field.h"
#include "atlas/interpolation/MissingValue.h"
#include "atlas/runtime/Exception.h"


namespace atlas {
namespace interpolation {
namespace nonlinear {


/**
 * @brief NonLinear class applies non-linear corrections to an interpolation matrix, given a field with missing values.
 * The interpolatation are re-weighted to factor those values out of the resulting field.
 */
class NonLinear {
public:
    using Config = eckit::Parametrisation;
    using Matrix = eckit::linalg::SparseMatrix;
    using Scalar = eckit::linalg::Scalar;
    using Size   = eckit::linalg::Size;

    /**
     * @brief NonLinear ctor
     */
    NonLinear() = default;

    /// @brief NonLinear dtor
    virtual ~NonLinear() = default;

    /**
     * @brief Apply non-linear corrections to interpolation matrix
     * @param [inout] W interpolation matrix
     * @param [in] f field with missing values information
     * @return if W was modified
     */
    virtual bool execute( Matrix& W, const Field& f ) const = 0;

protected:
    template <typename Value, int Rank>
    static array::ArrayView<typename std::add_const<Value>::type, Rank> make_view_field_values( const Field& field ) {
        ATLAS_ASSERT( field );
        ATLAS_ASSERT_MSG(
            field.datatype().kind() == array::DataType::kind<Value>(),
            "Field(name:" + field.name() + ",DataType:" + field.datatype().str() + ") is not of required DataType" );
        return array::make_view<typename std::add_const<Value>::type, Rank>( field );
    }
};


}  // namespace nonlinear
}  // namespace interpolation
}  // namespace atlas
