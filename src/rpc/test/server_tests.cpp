#include <rpc/server.h>
#include <test/test_bitcoin.h>
#include <util.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(server_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(server_IsDeprecatedRPCEnabled) {
    ArgsManager testArgs;
    testArgs.AddArg("-deprecatedrpc", "", false, OptionsCategory::OPTIONS);

    const char *argv_test[] = {"bitcoind", "-deprecatedrpc=foo",
                               "-deprecatedrpc=bar"};

    std::string error;
    BOOST_CHECK_MESSAGE(testArgs.ParseParameters(3, (char **)argv_test, error),
                        error);

    BOOST_CHECK(IsDeprecatedRPCEnabled(testArgs, "foo") == true);
    BOOST_CHECK(IsDeprecatedRPCEnabled(testArgs, "bar") == true);
    BOOST_CHECK(IsDeprecatedRPCEnabled(testArgs, "bob") == false);
}

BOOST_AUTO_TEST_SUITE_END()
