# Cold Rewards System Description

=============

Copyright (c) 2019 The DeVault developers/Copyright (c) 2019 Jon Spock
Distributed under the MIT software license, see http://www.opensource.org/licenses/mit-license.php.


# NOTE
**Cold Rewards are not generated for Coinbase transactions - which means that mining rewards, generated budget payments and cold rewards themselves will not generate Cold Rewards. You must move these to either another address or create a transaction to the same address to get rewards.**

# Overview
---------------------

Cold Rewards are a way for Coin holders to get rewarded for merely holding coins for approximately more than one month. The ROI is based on a scale that changes yearly and starts at 15% annualized. The wallet doesn't need to be kept open to get these rewards and action is generally not required (except by miners). There is also a minimum balance of 1000 DeVault needed at an unspent transaction output (UTXO). A transaction from an exchange or another person of greater than 1000 DeVault will achieve this. Please read below for more details. 


# Constants
---------------------

This is a list of constants used in this system

* BlocksPerYear    - Estimated number of blocks in 1 year
* PerCentPerYear   - Percentage return for Cold Rewards, currently changes each year from 15,12,9,7,5 %
* nMinRewardBlocks  - Rewards occur no more frequently than this
* nMinRewardBalance = 25000 * COIN after the 5th Superblock, 1000 * COIN before that
* nMinReward =  50 * COIN  - Only pay out reward if it's bigger than this

# Description
---------------------

Cold Rewards pays out coins to unspent transactions (UTXOs) that are greater than a minimum (**nMinReward** coins) and are older than approximately one month (**nMinRewardBlocks** number of blocks). NOTE: Addresses are not evaluated by total amounts, each separate UTXO is considered.

These unspent transaction must come from regular transactions. Either miner rewards or previous cold rewards will not counted - as coinbase outputs are ignored.

At each block all of the valid UTXOs will be evaluated for possible reward payout. We use a concept of "differential" height to determine viability. The "differential" height is the difference between the current block number and either 1) the block number when this UTXO was created or 2) the block number when this UTXO last got a rewards payout. That is, for the 1st payout, we will use 1) and afterwards 2). The coin address that will be rewarded will be the one with the biggest differential height, provided that the calculated reward is greater than (**nMinReward**).

If multiple UTXOs have the same differential height, the largest one will get paid out. If several UTXO have the same differential height and payout amount, then payout will be based on sorting the `COutpoints` (hex strings)

# Payout Amount
---------------------

**PerCentPerYear** determines the effective rate of return for payouts over 1 year. By dividing this by **BlocksPerYear** we'd get the return per block (as a very small number). We then multiply this by the "differential" height mentioned before to get the fractional return. We multiply by the balance at this UTXO to get the actual reward. Finally we quantize the reward to 1/10ths of a COIN for accounting simplicity and make sure it's greater than **nMinReward** to be considered valid.

# Invalidating Rewards
---------------------

For a given UTXO, once rewards are considered valid (given conditions already mentioned), they will be continued to be paid out as long as the UTXO itself is not spent. Once the UTXO is spent, rewards will cease.
This mechanism allows you to collect rewards and spend the actual rewards themselves since they will be at new UTXOs that will not be considered valid for rewards. To do that, you can use "Coin Control" feature within the wallet. If you want the rewards to earn rewards in return, you'd also need to use coin control to send the rewards using a regular transaction to any one of the addresses under your control. However, keep in mind that UTXOs considered valid must have more than **nMinRewardBalance** amounts. If you have more than that amount at one address but they are spread out across many UTXOs, you may not get rewards at all. You can use coin control to view your UTXO holdings. 











