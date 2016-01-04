/*
 * (C) Copyright 1996-2015 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <cmath>
#include <string>
#include <iterator>     // std::iterator, std::input_iterator_tag

#include "atlas/atlas_config.h"

#define BOOST_TEST_MODULE test_elements
#include "ecbuild/boost_test_framework.h"

#include "eckit/memory/ScopedPtr.h"
#include "eckit/exception/Exceptions.h"

#include "atlas/atlas.h"
#include "atlas/mesh/ElementType.h"
#include "atlas/mesh/Elements.h"
#include "atlas/mesh/Nodes.h"
#include "atlas/Field.h"
#include "atlas/Connectivity.h"

#include "atlas/Mesh.h"
#include "atlas/meshgen/ReducedGridMeshGenerator.h"
#include "atlas/Grid.h"
#include "atlas/functionspace/EdgeBasedFiniteVolume.h"

// ------------------------------------------------------------------

using namespace atlas::mesh;
using namespace atlas::mesh::temporary;

namespace atlas {
namespace test {


// ===================================================================
//                               BEGIN TESTS
// ===================================================================

struct GlobalFixture {
    GlobalFixture()  { atlas_init(boost::unit_test::framework::master_test_suite().argc,
                                  boost::unit_test::framework::master_test_suite().argv); }
    ~GlobalFixture() { atlas_finalize(); }
};

BOOST_GLOBAL_FIXTURE( GlobalFixture )

BOOST_AUTO_TEST_SUITE( test_elements )

BOOST_AUTO_TEST_CASE( hybrid_elements )
{
  HybridElements hybrid_elements;

  idx_t triangle_nodes[] = {
    1,5,3,
    1,5,2
  };
  size_t triags = hybrid_elements.add( new Triangle(), 2, triangle_nodes );

  hybrid_elements.add(Field::create<double>("surface",make_shape(hybrid_elements.size())));

  std::vector<idx_t> quad_nodes(4);
  quad_nodes[0] = 0;
  quad_nodes[1] = 1;
  quad_nodes[2] = 2;
  quad_nodes[3] = 3;
  size_t quads = hybrid_elements.add( new Quadrilateral(), 1, quad_nodes );


  {
    const HybridElements::Connectivity& connectivity = hybrid_elements.node_connectivity();
    for( size_t e=0; e<hybrid_elements.size(); ++e ) {
      eckit::Log::info() << e << std::endl;
      eckit::Log::info() << "  " << hybrid_elements.name(e) << std::endl;
      eckit::Log::info() << "  nb_nodes = " << hybrid_elements.nb_nodes(e) << std::endl;
      eckit::Log::info() << "  nb_edges = " << hybrid_elements.nb_edges(e) << std::endl;
      eckit::Log::info() << "  nodes = [ ";
      for( size_t n=0; n<hybrid_elements.nb_nodes(e); ++n ) {
        eckit::Log::info() << connectivity(e,n) << " ";
      }
      eckit::Log::info() << "]" << std::endl;
    }
    BOOST_CHECK_EQUAL( connectivity(0,0) , 1 );
    BOOST_CHECK_EQUAL( connectivity(0,1) , 5 );
    BOOST_CHECK_EQUAL( connectivity(0,2) , 3 );
    BOOST_CHECK_EQUAL( connectivity(1,0) , 1 );
    BOOST_CHECK_EQUAL( connectivity(1,1) , 5 );
    BOOST_CHECK_EQUAL( connectivity(1,2) , 2 );
    BOOST_CHECK_EQUAL( connectivity(2,0) , 0 );
    BOOST_CHECK_EQUAL( connectivity(2,1) , 1 );
    BOOST_CHECK_EQUAL( connectivity(2,2) , 2 );
    BOOST_CHECK_EQUAL( connectivity(2,3) , 3 );
  }

  eckit::Log::info() << std::endl;
  idx_t quad0[4] = {9,8,7,6};
  {
    for( size_t t=0; t<hybrid_elements.nb_types(); ++t ) {
      Elements& elements = hybrid_elements.elements(t);
      const Elements::Connectivity& block_connectivity = elements.node_connectivity();
      if( t==0 ) {
        BOOST_CHECK_EQUAL( block_connectivity(0,0) , 1 );
        BOOST_CHECK_EQUAL( block_connectivity(0,1) , 5 );
        BOOST_CHECK_EQUAL( block_connectivity(0,2) , 3 );
        BOOST_CHECK_EQUAL( block_connectivity(1,0) , 1 );
        BOOST_CHECK_EQUAL( block_connectivity(1,1) , 5 );
        BOOST_CHECK_EQUAL( block_connectivity(1,2) , 2 );
      }
      if( t==1 ) {
        BOOST_CHECK_EQUAL( block_connectivity(0,0) , 0 );
        BOOST_CHECK_EQUAL( block_connectivity(0,1) , 1 );
        BOOST_CHECK_EQUAL( block_connectivity(0,2) , 2 );
        BOOST_CHECK_EQUAL( block_connectivity(0,3) , 3 );
      }
      elements.node_connectivity().set(0,quad0);
      //const_cast<Elements::Connectivity&>(block_connectivity).set(0,quad0);
      //elements.set_node_connectivity(0,quad0);

      if( t==0 ) {
        BOOST_CHECK_EQUAL( block_connectivity(0,0) , 9 );
        BOOST_CHECK_EQUAL( block_connectivity(0,1) , 8 );
        BOOST_CHECK_EQUAL( block_connectivity(0,2) , 7 );
        BOOST_CHECK_EQUAL( block_connectivity(1,0) , 1 );
        BOOST_CHECK_EQUAL( block_connectivity(1,1) , 5 );
        BOOST_CHECK_EQUAL( block_connectivity(1,2) , 2 );
      }
      if( t==1 ) {
        BOOST_CHECK_EQUAL( block_connectivity(0,0) , 9 );
        BOOST_CHECK_EQUAL( block_connectivity(0,1) , 8 );
        BOOST_CHECK_EQUAL( block_connectivity(0,2) , 7 );
        BOOST_CHECK_EQUAL( block_connectivity(0,3) , 6 );
      }

      eckit::Log::info() << "name = " << elements.name() << std::endl;
      eckit::Log::info() << "nb_elements = " << elements.size() << std::endl;
      const Elements::Connectivity& connectivity = elements.node_connectivity();
      for( size_t e=0; e<elements.size(); ++e ) {
        eckit::Log::info() << "  nodes = [ ";
        for( size_t n=0; n<elements.nb_nodes(); ++n ) {
          eckit::Log::info() << connectivity(e,n) << " ";
        }
        eckit::Log::info() << "]" << std::endl;
      }
    }
  }

  size_t nb_elements = 3;
  BOOST_CHECK_EQUAL( hybrid_elements.size(),                  nb_elements);
  BOOST_CHECK_EQUAL( hybrid_elements.global_index().size(),   nb_elements);
  BOOST_CHECK_EQUAL( hybrid_elements.partition().size(),      nb_elements);
  BOOST_CHECK_EQUAL( hybrid_elements.remote_index().size(),   nb_elements);
  BOOST_CHECK_EQUAL( hybrid_elements.field("surface").size(), nb_elements);
  BOOST_CHECK_EQUAL( hybrid_elements.elements(0).begin(), 0);
  BOOST_CHECK_EQUAL( hybrid_elements.elements(0).end(),   2);
  BOOST_CHECK_EQUAL( hybrid_elements.elements(1).begin(), 2);
  BOOST_CHECK_EQUAL( hybrid_elements.elements(1).end(),   3);
  BOOST_CHECK_EQUAL( hybrid_elements.elements(0).type_idx(), 0 );
  BOOST_CHECK_EQUAL( hybrid_elements.elements(1).type_idx(), 1 );
}

BOOST_AUTO_TEST_CASE( elements )
{
  eckit::Log::info() << "\nelements" << std::endl;

  idx_t triangle_nodes[] = {
    1,5,3,
    1,5,2
  };
  idx_t triag1[3] = {9,8,7};

  Elements elements( new Triangle(), 2, triangle_nodes );

  BOOST_CHECK_EQUAL( elements.begin(), 0);
  BOOST_CHECK_EQUAL( elements.end(),   2);
  BOOST_CHECK_EQUAL( elements.type_idx(), 0);

  elements.node_connectivity().set(0,triag1);
  eckit::Log::info() << "name = " << elements.name() << std::endl;
  eckit::Log::info() << "nb_elements = " << elements.size() << std::endl;
  const Elements::Connectivity& connectivity = elements.node_connectivity();
  for( size_t e=0; e<elements.size(); ++e ) {
    eckit::Log::info() << "  nodes = [ ";
    for( size_t n=0; n<elements.nb_nodes(); ++n ) {
       eckit::Log::info() << connectivity(e,n) << " ";
    }
    eckit::Log::info() << "]" << std::endl;
  }

  // This will create a new structure with a copy of elements
  HybridElements hybrid_elements;
  hybrid_elements.add(elements);
  {
    for( size_t t=0; t<hybrid_elements.nb_types(); ++t ) {
      Elements& elements = hybrid_elements.elements(t);
      elements.node_connectivity().set(0,triag1);
      eckit::Log::info() << "name = " << elements.name() << std::endl;
      eckit::Log::info() << "nb_elements = " << elements.size() << std::endl;
      const Elements::Connectivity& connectivity = elements.node_connectivity();
      for( size_t e=0; e<elements.size(); ++e ) {
        eckit::Log::info() << "  nodes = [ ";
        for( size_t n=0; n<elements.nb_nodes(); ++n ) {
          eckit::Log::info() << connectivity(e,n) << " ";
        }
        eckit::Log::info() << "]" << std::endl;
      }
    }
  }
}

BOOST_AUTO_TEST_CASE( hybrid_connectivity )
{
  eckit::Log::info() << "\nhybrid_connectivity" << std::endl;

  idx_t triangle_nodes[] = {
    1,5,3,
    1,5,2
  };
  MultiBlockConnectivity hybrid_connectivity;
  hybrid_connectivity.add(2,3,triangle_nodes);
  for( size_t e=0; e<hybrid_connectivity.rows(); ++e )
  {
    eckit::Log::info() << "  cols = [ ";
    for( size_t n=0; n<hybrid_connectivity.cols(e); ++n ) {
      eckit::Log::info() << hybrid_connectivity(e,n) << " ";
    }
    eckit::Log::info() << "]" << std::endl;
  }


  idx_t quad_nodes[] = { 0,1,2,3,
                         4,5,6,7,
                         8,9,10,11 };
  hybrid_connectivity.add(3,4, quad_nodes);


  for( size_t e=0; e<hybrid_connectivity.rows(); ++e )
  {
    eckit::Log::info() << "  cols = [ ";
    for( size_t n=0; n<hybrid_connectivity.cols(e); ++n ) {
      eckit::Log::info() << hybrid_connectivity(e,n) << " ";
    }
    eckit::Log::info() << "]" << std::endl;
  }

  for( size_t b=0; b<hybrid_connectivity.blocks(); ++b )
  {
    const BlockConnectivity& block = hybrid_connectivity.block(b);
    for( size_t r=0; r<block.rows(); ++r )
    {
      eckit::Log::info() << "  cols = [ ";
      for( size_t c=0; c<block.cols(); ++c ) {
        eckit::Log::info() << block(r,c) << " ";
      }
      eckit::Log::info() << "]" << std::endl;
    }
  }


}


BOOST_AUTO_TEST_CASE( block_connectivity )
{
  eckit::Log::info() << "\nblock_connectivity" << std::endl;

  idx_t triangle_nodes[] = {
    1,5,3,
    1,5,2
  };
  BlockConnectivity block;
  block.add(2,3,triangle_nodes);
  block.add(2,3,triangle_nodes);
  for( size_t r=0; r<block.rows(); ++r )
  {
    eckit::Log::info() << "  cols = [ ";
    for( size_t c=0; c<block.cols(); ++c ) {
      eckit::Log::info() << block(r,c) << " ";
    }
    eckit::Log::info() << "]" << std::endl;
  }

}

BOOST_AUTO_TEST_CASE( zero_elements )
{
  HybridElements hybrid_elements;
  idx_t *nodes = 0;
  hybrid_elements.add(new Triangle(), 0, nodes );
  hybrid_elements.add(new Quadrilateral(), 0, nodes );
  BOOST_CHECK_EQUAL( hybrid_elements.size(), 0 );
  BOOST_CHECK_EQUAL( hybrid_elements.nb_types(), 2 );
  BOOST_CHECK_EQUAL( hybrid_elements.elements(0).size(), 0 );
  BOOST_CHECK_EQUAL( hybrid_elements.elements(1).size(), 0 );
}

BOOST_AUTO_TEST_CASE( conversion )
{
  long nlon[] = {4,8};
  Grid* grid = Grid::create("N16");
  meshgen::ReducedGridMeshGenerator generator;
  generator.options.set("angle",29.0);
  generator.options.set("triangulate",false);
  Mesh* mesh = generator.generate(*grid);
  
  next::FunctionSpace* functionspace = new functionspace::EdgeBasedFiniteVolume(*mesh);
  
  temporary::Convert::convertMesh( *mesh );
  
  const HybridElements& edges = mesh->edges();
  const HybridElements& cells = mesh->cells();
  const Nodes& nodes          = mesh->nodes();
  
  DEBUG_VAR( nodes.size() );
  DEBUG_VAR( cells.size() );
  DEBUG_VAR( cells.elements(0).size() );
  DEBUG_VAR( cells.elements(1).size() );
  DEBUG_VAR( edges.size() );
  
  eckit::Log::info() << "edges\n";
  for( size_t f=0; f<edges.size(); ++f )
  {
    eckit::Log::info() << f << " : ";
    // for( size_t n=0; n<edges.nb_nodes(f); ++n )
    // {
    //   eckit::Log::info() << edges.node_connectivity()(f,n) << " ";
    // }
    for( size_t e=0; e<edges.cell_connectivity().cols(f); ++e )
    {
      eckit::Log::info() << edges.cell_connectivity()(f,e) << " ";
    }
    eckit::Log::info() << "\n";
  }
  eckit::Log::info() << std::flush;
  //
  //
  // eckit::Log::info() << "cells\n";
  // for( size_t e=0; e<cells.size(); ++e )
  // {
  //   eckit::Log::info() << e << " : ";
  //   for( size_t n=0; n<cells.nb_nodes(e); ++n )
  //   {
  //     eckit::Log::info() << cells.node_connectivity()(e,n) << " ";
  //   }
  //   eckit::Log::info() << "\n";
  // }
  // eckit::Log::info() << std::flush;

  // BOOST_CHECKPOINT("start edge_connectivity");
  // eckit::Log::info() << "cells\n";
  // for( size_t e=0; e<cells.edge_connectivity().rows(); ++e )
  // {
  //   eckit::Log::info() << e << " : ";
  //   for( size_t n=0; n<cells.edge_connectivity().cols(e); ++n )
  //   {
  //     eckit::Log::info() << cells.edge_connectivity()(e,n) << " ";
  //   }
  //   eckit::Log::info() << "\n";
  // }
  // eckit::Log::info() << std::flush;
  
  ArrayView<double,2> centroids( cells.field("centroids") );
  for( size_t e=cells.elements(1).begin(); e<cells.elements(1).end(); ++e )
  {
    // DEBUG_VAR( centroids(e,XX) );
    // DEBUG_VAR( centroids(e,YY) );
    std::set<idx_t> cell_nodes;
    std::set<idx_t> edge_nodes;
    cell_nodes.insert(cells.node_connectivity()(e,0));
    cell_nodes.insert(cells.node_connectivity()(e,1));
    cell_nodes.insert(cells.node_connectivity()(e,2));
  
    edge_nodes.insert( edges.node_connectivity()( cells.edge_connectivity()(e,0), 0) );
    edge_nodes.insert( edges.node_connectivity()( cells.edge_connectivity()(e,0), 1) );
    edge_nodes.insert( edges.node_connectivity()( cells.edge_connectivity()(e,1), 0) );
    edge_nodes.insert( edges.node_connectivity()( cells.edge_connectivity()(e,1), 1) );
    edge_nodes.insert( edges.node_connectivity()( cells.edge_connectivity()(e,2), 0) );
    edge_nodes.insert( edges.node_connectivity()( cells.edge_connectivity()(e,2), 1) );
    BOOST_CHECK_EQUAL_COLLECTIONS( cell_nodes.begin(),cell_nodes.end(), edge_nodes.begin(), edge_nodes.end() );
  }
  
  
  delete functionspace;
  delete mesh;
  delete grid;
}



BOOST_AUTO_TEST_SUITE_END()

} // namespace test
} // namespace atlas
