#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test multiwallet.

Verify that a bitcoind node can load multiple wallet files
"""
import os
import re
import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class MultiWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [
            ['-wallet=w1', '-wallet=w2', '-wallet=w3', '-wallet=w']]
        self.supports_cli = True

    def run_test(self):
        node = self.nodes[0]

        data_dir = lambda *p: os.path.join(node.datadir, 'regtest', *p)
        wallet_dir = lambda *p: data_dir('wallets', *p)

        def wallet(name): return node.get_wallet_rpc(name)

        assert_equal(set(node.listwallets()), {"w1", "w2", "w3", "w"})

        self.stop_node(0)
        for wallet_name in wallet_names:
            if os.path.isdir(wallet_dir(wallet_name)):
                assert_equal(os.path.isfile(
                    wallet_dir(wallet_name, "wallet.dat")), True)
            else:
                assert_equal(os.path.isfile(wallet_dir(wallet_name)), True)

        # should not initialize if wallet path can't be created
        exp_stderr = "boost::filesystem::create_directory: (The system cannot find the path specified|Not a directory):"
        self.nodes[0].assert_start_raises_init_error(
            ['-wallet=wallet.dat/bad'], exp_stderr, partial_match=True)

        self.nodes[0].assert_start_raises_init_error(
            ['-walletdir=wallets'], 'Error: Specified -walletdir "wallets" does not exist')
        self.nodes[0].assert_start_raises_init_error(
            ['-walletdir=wallets'], 'Error: Specified -walletdir "wallets" is a relative path', cwd=data_dir())
        self.nodes[0].assert_start_raises_init_error(
            ['-walletdir=debug.log'], 'Error: Specified -walletdir "debug.log" is not a directory', cwd=data_dir())

        # should not initialize if there are duplicate wallets
        self.nodes[0].assert_start_raises_init_error(
            ['-wallet=w1', '-wallet=w1'], 'Error: Error loading wallet w1. Duplicate -wallet filename specified.')

        # should not initialize if wallet file is a directory
        os.mkdir(wallet_dir('w11'))
        self.assert_start_raises_init_error(
            0, ['-wallet=w11'], 'Error loading wallet w11. -wallet filename must be a regular file.')

        # should not initialize if one wallet is a copy of another
        shutil.copyfile(wallet_dir('w2'), wallet_dir('w22'))
        self.assert_start_raises_init_error(
            0, ['-wallet=w2', '-wallet=w22'], 'duplicates fileid')

        # should not initialize if wallet file is a symlink
        os.symlink(wallet_dir('w1'), wallet_dir('w12'))
        self.assert_start_raises_init_error(
            0, ['-wallet=w12'], 'Error loading wallet w12. -wallet filename must be a regular file.')

        # should not initialize if the specified walletdir does not exist
        self.nodes[0].assert_start_raises_init_error(
            ['-walletdir=bad'], 'Error: Specified -walletdir "bad" does not exist')
        # should not initialize if the specified walletdir is not a directory
        not_a_dir = wallet_dir('notadir')
        open(not_a_dir, 'a').close()
        self.nodes[0].assert_start_raises_init_error(
            ['-walletdir=' + not_a_dir], 'Error: Specified -walletdir "' + re.escape(not_a_dir) + '" is not a directory')

        # if wallets/ doesn't exist, datadir should be the default wallet dir
        wallet_dir2 = data_dir('walletdir')
        os.rename(wallet_dir(), wallet_dir2)
        self.start_node(0, ['-wallet=w4', '-wallet=w5'])
        assert_equal(set(node.listwallets()), {"w4", "w5"})
        w5 = wallet("w5")
        w5.generate(1)
        self.stop_node(0)

        # now if wallets/ exists again, but the rootdir is specified as the walletdir, w4 and w5 should still be loaded
        os.rename(wallet_dir2, wallet_dir())
        self.start_node(0, ['-wallet=w4', '-wallet=w5',
                            '-walletdir=' + data_dir()])
        assert_equal(set(node.listwallets()), {"w4", "w5"})
        w5 = wallet("w5")
        w5_info = w5.getwalletinfo()
        assert_equal(w5_info['immature_balance'], 50)

        self.stop_node(0)

        self.start_node(0, self.extra_args[0])

        w1 = wallet("w1")
        w2 = wallet("w2")
        w3 = wallet("w3")
        w4 = wallet("w")
        wallet_bad = wallet("bad")

        w1.generate(1)

        # accessing invalid wallet fails
        assert_raises_rpc_error(-18, "Requested wallet does not exist or is not loaded",
                                wallet_bad.getwalletinfo)

        # accessing wallet RPC without using wallet endpoint fails
        assert_raises_rpc_error(-19, "Wallet file not specified (must request wallet RPC through /wallet/<filename> uri-path).",
                                node.getwalletinfo)

        # check w1 wallet balance
        w1_info = w1.getwalletinfo()
        assert_equal(w1_info['immature_balance'], 50)
        w1_name = w1_info['walletname']
        assert_equal(w1_name, "w1")

        # check w2 wallet balance
        w2_info = w2.getwalletinfo()
        assert_equal(w2_info['immature_balance'], 0)
        w2_name = w2_info['walletname']
        assert_equal(w2_name, "w2")

        w3_name = w3.getwalletinfo()['walletname']
        assert_equal(w3_name, "w3")

        w4_name = w4.getwalletinfo()['walletname']
        assert_equal(w4_name, "w")

        w1.generate(101)
        assert_equal(w1.getbalance(), 100)
        assert_equal(w2.getbalance(), 0)
        assert_equal(w3.getbalance(), 0)
        assert_equal(w4.getbalance(), 0)

        w1.sendtoaddress(w2.getnewaddress(), 1)
        w1.sendtoaddress(w3.getnewaddress(), 2)
        w1.sendtoaddress(w4.getnewaddress(), 3)
        w1.generate(1)
        assert_equal(w2.getbalance(), 1)
        assert_equal(w3.getbalance(), 2)
        assert_equal(w4.getbalance(), 3)

        batch = w1.batch([w1.getblockchaininfo.get_request(),
                          w1.getwalletinfo.get_request()])
        assert_equal(batch[0]["result"]["chain"], "regtest")
        assert_equal(batch[1]["result"]["walletname"], "w1")

        self.log.info('Check for per-wallet settxfee call')
        assert_equal(w1.getwalletinfo()['paytxfee'], 0)
        assert_equal(w2.getwalletinfo()['paytxfee'], 0)
        w2.settxfee(4.0)
        assert_equal(w1.getwalletinfo()['paytxfee'], 0)
        assert_equal(w2.getwalletinfo()['paytxfee'], 4.0)

        self.log.info("Test dynamic wallet loading")

        self.restart_node(0, ['-nowallet'])
        assert_equal(node.listwallets(), [])
        assert_raises_rpc_error(-32601, "Method not found", node.getwalletinfo)

        self.log.info("Load first wallet")
        loadwallet_name = node.loadwallet(wallet_names[0])
        assert_equal(loadwallet_name['name'], wallet_names[0])
        assert_equal(node.listwallets(), wallet_names[0:1])
        node.getwalletinfo()
        w1 = node.get_wallet_rpc(wallet_names[0])
        w1.getwalletinfo()

        self.log.info("Load second wallet")
        loadwallet_name = node.loadwallet(wallet_names[1])
        assert_equal(loadwallet_name['name'], wallet_names[1])
        assert_equal(node.listwallets(), wallet_names[0:2])
        assert_raises_rpc_error(-19,
                                "Wallet file not specified", node.getwalletinfo)
        w2 = node.get_wallet_rpc(wallet_names[1])
        w2.getwalletinfo()

        self.log.info("Load remaining wallets")
        for wallet_name in wallet_names[2:]:
            loadwallet_name = self.nodes[0].loadwallet(wallet_name)
            assert_equal(loadwallet_name['name'], wallet_name)

        assert_equal(set(self.nodes[0].listwallets()), set(wallet_names))

        # Fail to load if wallet doesn't exist
        assert_raises_rpc_error(-18, 'Wallet wallets not found.',
                                self.nodes[0].loadwallet, 'wallets')

        # Fail to load duplicate wallets
        assert_raises_rpc_error(-4, 'Wallet file verification failed: Error loading wallet w1. Duplicate -wallet filename specified.',
                                self.nodes[0].loadwallet, wallet_names[0])

        # Fail to load duplicate wallets by different ways (directory and filepath)
        assert_raises_rpc_error(-4, "Wallet file verification failed: Error loading wallet wallet.dat. Duplicate -wallet filename specified.",
                                self.nodes[0].loadwallet, 'wallet.dat')

        # Fail to load if one wallet is a copy of another
        assert_raises_rpc_error(-1, "BerkeleyBatch: Can't open database w8_copy (duplicates fileid",
                                self.nodes[0].loadwallet, 'w8_copy')

        # Fail to load if one wallet is a copy of another.
        # Test this twice to make sure that we don't re-introduce https://github.com/bitcoin/bitcoin/issues/14304
        assert_raises_rpc_error(-1, "BerkeleyBatch: Can't open database w8_copy (duplicates fileid",
                                self.nodes[0].loadwallet, 'w8_copy')

        # Fail to load if wallet file is a symlink
        assert_raises_rpc_error(-4, "Wallet file verification failed: Invalid -wallet path 'w8_symlink'",
                                self.nodes[0].loadwallet, 'w8_symlink')

        self.log.info("Test dynamic wallet creation.")

        # Fail to create a wallet if it already exists.
        assert_raises_rpc_error(-4, "Wallet w2 already exists.",
                                self.nodes[0].createwallet, 'w2')

        # Successfully create a wallet with a new name
        loadwallet_name = self.nodes[0].createwallet('w9')
        assert_equal(loadwallet_name['name'], 'w9')
        w9 = node.get_wallet_rpc('w9')
        assert_equal(w9.getwalletinfo()['walletname'], 'w9')

        assert 'w9' in self.nodes[0].listwallets()

        # Successfully create a wallet using a full path
        new_wallet_dir = os.path.join(self.options.tmpdir, 'new_walletdir')
        new_wallet_name = os.path.join(new_wallet_dir, 'w10')
        loadwallet_name = self.nodes[0].createwallet(new_wallet_name)
        assert_equal(loadwallet_name['name'], new_wallet_name)
        w10 = node.get_wallet_rpc(new_wallet_name)
        assert_equal(w10.getwalletinfo()['walletname'], new_wallet_name)

        assert new_wallet_name in self.nodes[0].listwallets()


if __name__ == '__main__':
    MultiWalletTest().main()
