#pragma once

#define BOOST_CHECK CHECK
#define BOOST_REQUIRE REQUIRE

#define BOOST_CHECK_EQUAL(A,B) CHECK(A == B)
#define BOOST_REQUIRE_EQUAL(A,B) REQUIRE(A == B)

#define BOOST_CHECK_THROW(A,B) CHECK_THROWS_AS(A,B)
#define BOOST_CHECK_NO_THROW CHECK_NOTHROW

#define BOOST_CHECK_MESSAGE(A,B) CHECK(A); if (!(A)) std::cout << B << "\n";

#define BOOST_ERROR(A) std::cout << (A) << "\n";

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
