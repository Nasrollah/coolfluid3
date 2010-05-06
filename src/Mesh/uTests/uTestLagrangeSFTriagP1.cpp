#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "Test module for CF::Mesh::LagrangeSF"

#include <boost/assign/list_of.hpp>
#include <boost/test/unit_test.hpp>

#include "Common/Log.hpp"
#include "Common/CRoot.hpp"
#include "Mesh/Integrators/Gauss.hpp"
#include "Mesh/LagrangeSF/TriagP1.hpp"
#include "Mesh/P1/Triag2D.hpp"
#include "Tools/Difference/Difference.hpp"

using namespace boost::assign;
using namespace CF::Mesh::Integrators;
using namespace CF::Mesh::LagrangeSF;
using namespace CF::Mesh::P1;

//////////////////////////////////////////////////////////////////////////////

struct LagrangeSFTriagP1_Fixture
{
  /// common setup for each test case
  LagrangeSFTriagP1_Fixture() : mapped_coords(init_mapped_coords()), nodes(init_nodes()), nodes_ptr(init_nodes_ptr())
  {
     // uncomment if you want to use arguments to the test executable
     //int*    argc = &boost::unit_test::framework::master_test_suite().argc;
     //char*** argv = &boost::unit_test::framework::master_test_suite().argv;

    //mapped_coords += 0.1, 0.8;
  }

  /// common tear-down for each test case
  ~LagrangeSFTriagP1_Fixture()
  {
  }
  /// common values accessed by all tests goes here

  const CF::RealVector mapped_coords;
  const NodesT nodes;
  const std::vector<CF::RealVector*> nodes_ptr;

  template<typename ShapeF>
  struct const_functor
  {
    const_functor(const NodesT& node_list) : m_nodes(node_list) {}
    CF::Real operator()(const CF::RealVector& mappedCoords)
    {
      return ShapeF::computeJacobianDeterminant(mappedCoords, m_nodes);
    }
  private:
    const NodesT& m_nodes;
  };

private:
  /// Workaround for boost:assign ambiguity
  CF::RealVector init_mapped_coords()
  {
    return list_of(0.1)(0.8);
  }

  /// Workaround for boost:assign ambiguity
  NodesT init_nodes()
  {
    const CF::RealVector c0 = list_of(0.5)(0.3);
    const CF::RealVector c1 = list_of(1.1)(1.2);
    const CF::RealVector c2 = list_of(0.8)(2.1);
    return list_of(c0)(c1)(c2);
  }

  /// Workaround for boost:assign ambiguity
  std::vector<CF::RealVector*> init_nodes_ptr()
  {
    return list_of(&nodes[0])(&nodes[1])(&nodes[2]);
  }

};

//////////////////////////////////////////////////////////////////////////////

BOOST_FIXTURE_TEST_SUITE( LagrangeSFTriagP1, LagrangeSFTriagP1_Fixture )

//////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( computeShapeFunction )
{
  const CF::RealVector reference_result = list_of(0.1)(0.1)(0.8);
  CF::RealVector result(3);
  TriagP1::computeShapeFunction(mapped_coords, result);
  CF::Tools::Difference::Accumulator accumulator;
  CF::Tools::Difference::vector_test(result, reference_result, accumulator);
  BOOST_CHECK_LT(boost::accumulators::max(accumulator.ulps), 10); // Maximal difference can't be greater than 10 times the least representable unit
}

BOOST_AUTO_TEST_CASE( computeMappedCoordinates )
{
  const CF::RealVector test_coords = list_of(0.8)(1.2);
  const CF::RealVector reference_result = list_of(1./3.)(1./3.);
  CF::RealVector result(2);
  TriagP1::computeMappedCoordinates(test_coords, nodes, result);
  CF::Tools::Difference::Accumulator accumulator;
  CF::Tools::Difference::vector_test(result, reference_result, accumulator);
  BOOST_CHECK_LT(boost::accumulators::max(accumulator.ulps), 10); // Maximal difference can't be greater than 10 times the least representable unit
}

BOOST_AUTO_TEST_CASE( integrateConst )
{
  const_functor<TriagP1> ftor(nodes);
  CF::Real result = 0.0;
  Gauss<TriagP1>::integrate(ftor, result);
  BOOST_CHECK_LT(boost::accumulators::max(CF::Tools::Difference::test(result, CF::Mesh::VolumeComputer<Triag2D>::computeVolume(nodes_ptr)).ulps), 1);
}

BOOST_AUTO_TEST_CASE( computeMappedGradient )
{
  CF::RealMatrix expected(3, 2);
  expected(0,0) = -1.;
  expected(0,1) = -1.;
  expected(1,0) = 1.;
  expected(1,1) = 0.;
  expected(2,0) = 0.;
  expected(2,1) = 1.;
  CF::RealMatrix result(3, 2);
  TriagP1::computeMappedGradient(mapped_coords, result);
  CF::Tools::Difference::Accumulator accumulator;
  CF::Tools::Difference::vector_test(result, expected, accumulator);
  BOOST_CHECK_LT(boost::accumulators::max(accumulator.ulps), 2);
}

BOOST_AUTO_TEST_CASE( computeJacobianDeterminant )
{
  // Shapefunction determinant should be double the volume for triangles
  BOOST_CHECK_LT(boost::accumulators::max(CF::Tools::Difference::test(0.5*TriagP1::computeJacobianDeterminant(mapped_coords, nodes), CF::Mesh::VolumeComputer<Triag2D>::computeVolume(nodes_ptr)).ulps), 1);
}

BOOST_AUTO_TEST_CASE( computeJacobian )
{
  CF::RealMatrix expected(2, 2);
  expected(0,0) = 0.6;
  expected(0,1) = 0.9;
  expected(1,0) = 0.3;
  expected(1,1) = 1.8;
  CF::RealMatrix result(2, 2);
  TriagP1::computeJacobian(mapped_coords, nodes, result);
  CF::Tools::Difference::Accumulator accumulator;
  CF::Tools::Difference::vector_test(result, expected, accumulator);
  BOOST_CHECK_LT(boost::accumulators::max(accumulator.ulps), 2);
}

BOOST_AUTO_TEST_CASE( computeJacobianAdjoint )
{
  CF::RealMatrix expected(2, 2);
  expected(0,0) = 1.8;
  expected(0,1) = -0.9;
  expected(1,0) = -0.3;
  expected(1,1) = 0.6;
  CF::RealMatrix result(2, 2);
  TriagP1::computeJacobianAdjoint(mapped_coords, nodes, result);
  CF::Tools::Difference::Accumulator accumulator;
  CF::Tools::Difference::vector_test(result, expected, accumulator);
  BOOST_CHECK_LT(boost::accumulators::max(accumulator.ulps), 2);
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE_END()

////////////////////////////////////////////////////////////////////////////////

