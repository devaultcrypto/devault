# Budget Description

=============

Example of Budget Struct (to be update with actual addresses/amounts later)

const BudgetStruct Payouts[] = {
    {"devault:qryvj8ezelgrgmr22myc7suwnn7pal9a0vtvtwfr96", "Community",15},
    {"devault:qpd3d3ufpcwa2n6ghqnmlrcw62syh68cjvc98u84ws", "CoreDevs", 10},
    {"devault:qz4nj7c6x5uf5ar6rqjj602eyux26t0ftuarvygttl", "WebDevs", 5},
    {"devault:qzeyacx0xhvpd0zq7neyywed0qs4yfh2psr36ntaep", "BusDevs",5},
    {"devault:qzjjtr6pxrkh69ugsvjcwj4h7dexvx72ayh3vkgrwt",, "Marketing",5},
    {"devault:qpd3d3ufpcwa2n6ghqnmlrcw62syh68cjvc98u84ws", "Support", 5}};

# Description
---------------------

Budget payouts occur every Superblock. A Superblock occurs every 21916 blocks, which is roughly once per month or 12 times a year.

Currently we have 6 budget items with the following names and % of the block reward

* Community projects - 15%
* Core Development - 10% 
* Web Development - 5%
* Business Development - 5%
* Marketing - 5%
* User Support - 5%

The percentages are calculated by looking at the block reward at the time of the Superblock and then determining how many coins would go to each of the above assuming that block reward was constant for the last 21916 blocks. So for example if 100000 coins went to Core Development, you could assume that 1 million coins were generated for both the Budget and Mining over the previous month (since Core is 10%). Since in our case, mining rewards can increase and decrease each block this will not be completely accurate. Since the total Budget amount is 45%, then the mining reward is equivalent to 55%















