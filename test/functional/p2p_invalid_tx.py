#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import time

from test_framework.blocktools import (
    create_block,
    create_coinbase,
    create_transaction,
)
from test_framework.comptool import RejectResult, TestInstance, TestManager
from test_framework.messages import COIN
from test_framework.mininode import network_thread_start
from test_framework.test_framework import ComparisonTestFramework


'''
In this test we connect to one node over p2p, and test tx requests.
'''
REJECT_INVALID = 16
REJECT_PUSHONLY = b'mandatory-script-verify-flag-failed (Only push operators allowed in signature scripts)'


# Use the ComparisonTestFramework with 1 node: only use --testbinary.


class InvalidTxRequestTest(ComparisonTestFramework):

    ''' Can either run this test as 1 node with expected answers, or two and compare them. 
        Change the "outcome" variable from each TestInstance object to only do the comparison. '''

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        test = TestManager(self, self.options.tmpdir)
        test.add_all_connections(self.nodes)
        self.tip = None
        self.block_time = None
        network_thread_start()
        test.run()

    def get_tests(self):
        if self.tip is None:
            self.tip = int("0x" + self.nodes[0].getbestblockhash(), 0)
        self.block_time = int(time.time()) + 1

        '''
        Create a new block with an anyone-can-spend coinbase
        '''
        height = 1
        block = create_block(
            self.tip, create_coinbase(height), self.block_time)
        self.block_time += 1
        block.solve()
        # Save the coinbase for later
        self.block1 = block
        self.tip = block.sha256
        height += 1
        yield TestInstance([[block, True]])

        '''
        Now we need that block to mature so we can spend the coinbase.
        '''
        test = TestInstance(sync_every_block=False)
        for i in range(100):
            block = create_block(
                self.tip, create_coinbase(height), self.block_time)
            block.solve()
            self.tip = block.sha256
            self.block_time += 1
            test.blocks_and_transactions.append([block, True])
            height += 1
        yield test

        # b'\x64' is OP_NOTIF
        # Transaction will be rejected with code 16 (REJECT_INVALID)
        # and we get disconnected immediately
        self.log.info('Test a transaction that is rejected')
        tx1 = create_transaction(
            block1.vtx[0], 0, b'\x64' * 35, 50 * COIN - 12000)
        node.p2p.send_txs_and_test([tx1], node, success=False, expect_disconnect=True,
                                   reject_code=REJECT_INVALID, reject_reason=REJECT_PUSHONLY)

        # Make two p2p connections to provide the node with orphans
        # * p2ps[0] will send valid orphan txs (one with low fee)
        # * p2ps[1] will send an invalid orphan tx (and is later disconnected for that)
        self.reconnect_p2p(num_connections=2)

        self.log.info('Test orphan transaction handling ... ')
        # Create a root transaction that we withold until all dependend transactions
        # are sent out and in the orphan cache
        tx_withhold = CTransaction()
        tx_withhold.vin.append(
            CTxIn(outpoint=COutPoint(block1.vtx[0].sha256, 0)))
        tx_withhold.vout.append(
            CTxOut(nValue=50 * COIN - 12000, scriptPubKey=b'\x51'))
        pad_tx(tx_withhold)
        tx_withhold.calc_sha256()

        # Our first orphan tx with some outputs to create further orphan txs
        tx_orphan_1 = CTransaction()
        tx_orphan_1.vin.append(
            CTxIn(outpoint=COutPoint(tx_withhold.sha256, 0)))
        tx_orphan_1.vout = [CTxOut(nValue=10 * COIN, scriptPubKey=b'\x51')] * 3
        pad_tx(tx_orphan_1)
        tx_orphan_1.calc_sha256()

        # A valid transaction with low fee
        tx_orphan_2_no_fee = CTransaction()
        tx_orphan_2_no_fee.vin.append(
            CTxIn(outpoint=COutPoint(tx_orphan_1.sha256, 0)))
        tx_orphan_2_no_fee.vout.append(
            CTxOut(nValue=10 * COIN, scriptPubKey=b'\x51'))
        pad_tx(tx_orphan_2_no_fee)

        # A valid transaction with sufficient fee
        tx_orphan_2_valid = CTransaction()
        tx_orphan_2_valid.vin.append(
            CTxIn(outpoint=COutPoint(tx_orphan_1.sha256, 1)))
        tx_orphan_2_valid.vout.append(
            CTxOut(nValue=10 * COIN - 12000, scriptPubKey=b'\x51'))
        tx_orphan_2_valid.calc_sha256()
        pad_tx(tx_orphan_2_valid)

        # An invalid transaction with negative fee
        tx_orphan_2_invalid = CTransaction()
        tx_orphan_2_invalid.vin.append(
            CTxIn(outpoint=COutPoint(tx_orphan_1.sha256, 2)))
        tx_orphan_2_invalid.vout.append(
            CTxOut(nValue=11 * COIN, scriptPubKey=b'\x51'))
        pad_tx(tx_orphan_2_invalid)

        self.log.info('Send the orphans ... ')
        # Send valid orphan txs from p2ps[0]
        node.p2p.send_txs_and_test(
            [tx_orphan_1, tx_orphan_2_no_fee, tx_orphan_2_valid], node, success=False)
        # Send invalid tx from p2ps[1]
        node.p2ps[1].send_txs_and_test(
            [tx_orphan_2_invalid], node, success=False)

        # Mempool should be empty
        assert_equal(0, node.getmempoolinfo()['size'])
        assert_equal(2, len(node.getpeerinfo()))  # p2ps[1] is still connected

        self.log.info('Send the withhold tx ... ')
        node.p2p.send_txs_and_test([tx_withhold], node, success=True)

        # Transactions that should end up in the mempool
        expected_mempool = {
            t.hash
            for t in [
                tx_withhold,  # The transaction that is the root for all orphans
                tx_orphan_1,  # The orphan transaction that splits the coins
                # The valid transaction (with sufficient fee)
                tx_orphan_2_valid,
            ]
        }
        # Transactions that do not end up in the mempool
        # tx_orphan_no_fee, because it has too low fee (p2ps[0] is not disconnected for relaying that tx)
        # tx_orphan_invaid, because it has negative fee (p2ps[1] is disconnected for relaying that tx)

        # p2ps[1] is no longer connected
        wait_until(lambda: 1 == len(node.getpeerinfo()), timeout=12)
        assert_equal(expected_mempool, set(node.getrawmempool()))

        # restart node with sending BIP61 messages disabled, check that it disconnects without sending the reject message
        self.log.info(
            'Test a transaction that is rejected, with BIP61 disabled')
        self.restart_node(0, ['-enablebip61=0', '-persistmempool=0'])
        self.reconnect_p2p(num_connections=1)
        node.p2p.send_txs_and_test(
            [tx1], node, success=False, expect_disconnect=True)
        # send_txs_and_test will have waited for disconnect, so we can safely check that no reject has been received
        assert_equal(node.p2p.reject_code_received, None)


if __name__ == '__main__':
    InvalidTxRequestTest().main()
