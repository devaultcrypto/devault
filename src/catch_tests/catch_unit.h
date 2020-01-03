#pragma once

#define BOOST_CHECK CHECK
#define BOOST_REQUIRE REQUIRE

#define BOOST_CHECK_EQUAL(A,B) CHECK(A == B)
#define BOOST_REQUIRE_EQUAL(A,B) REQUIRE(A == B)

#define BOOST_CHECK_THROW(A,B) CHECK_THROWS_AS(A,B)
#define BOOST_CHECK_NO_THROW CHECK_NOTHROW

#define BOOST_CHECK_MESSAGE(A,B) CHECK(A); if (!(A)) std::cout << B << "\n";

#define BOOST_ERROR(A) std::cout << (A) << "\n";

#define BOOST_FIXTURE_TEST_SUITE(a,b)
#define BOOST_AUTO_TEST_SUITE_END()

#define STR_VALUE(arg)      #arg
// this extra level may not be needed
#define XXX_CASE TEST_CASE
#define BOOST_AUTO_TEST_CASE(name) XXX_CASE(STR_VALUE(name))

#define BOOST_TEST_MESSAGE(a) std::cout << a

#include <catch2/catch.hpp>

/**
 * Version of Boost::test prior to 1.64 have issues when dealing with nullptr_t.
 * In order to work around this, we ensure that the null pointers are typed in a
 * way that Boost will like better.
 *
 * TODO: Use nullptr directly once the minimum version of boost is 1.64 or more.
 */
#define NULLPTR(T) static_cast<T *>(nullptr)
