
# Reorg testplan

Increment Protocol to ban other nodes
Set   consensus.nBlocksPerYear = 200;
So that Rewards happen 25 blocks.

# Node 1.

Generate 150 blocks

At block 150, Make 2 payments to CRa and CRb
Generate 1 block
Then make 2 more payments to CRc and CRd
Generate blocks such that the 4 CRs are paid out. You'll now know the sequence of these 4 rewards (depends on addr strings).
Call the sequence CR1, CR2, CR3, CR4
Generate about 20 more blocks, lets say up to block 199

# Node 2.
node sync up to this point and then disconnect the nodes

# Node 1.

Generate blocks until the 1st CR is paid out (assume it's CR1)
Now spend CR2 and CR3
Generate another block
after another block or two you should see a payout for CR4
generate another block, lets say you are now at 210


# Node 2
generate block past 210. There is no need for this node to do anything else but wallet can be shared if desired.


Reconnect nodes so the Node 1 Resyncs to Node 2

# Node 1 should re-org.

Both nodes should be able to generate blocks without errors

Make sure you generate another 50 blocks and all is ok









