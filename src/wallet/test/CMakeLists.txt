# Copyright (c) 2019 The DeVault developers
# Copyright (c) 2019 Jon Spock
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

project(wallet_tests)

SET(WALLET_CTESTS  
	accounting
	bip39
	wallet
	walletdb
	)

FOREACH(tst ${WALLET_CTESTS})
	set(SRC ${tst}_tests.cpp wallet_test_fixture.cpp)
	add_executable(tb_${tst} ${SRC})
  target_link_libraries(tb_${tst} rpcclient server unit bls relic_s)
  add_test(NAME test_wallet_${tst} COMMAND tb_${tst})
ENDFOREACH(tst ${WALLET_CTESTS})

