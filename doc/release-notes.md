This release includes the following features and fixes since 1.2.1 release:

 - Fix bug in rpc method verifymessage
 - increase min change (+ min final change)
 - Change some CMake build defaults
 - `listwalletdir` returns a list of wallets in the wallet directory which is
   configured with `-walletdir` parameter.

This release includes the following features and fixes:

'label' and 'account' APIs for wallet
-------------------------------------

A new 'label' API has been introduced for the wallet. This is intended as a
replacement for the deprecated 'account' API. The 'account' can continue to
be used in v0.20 by starting bitcoind with the '-deprecatedrpc=accounts'
argument, and will be fully removed in v0.21.

The label RPC methods mirror the account functionality, with the following functional differences:

- Labels can be set on any address, not just receiving addresses. This functionality was previously only available through the GUI.
- Labels can be deleted by reassigning all addresses using the `setlabel` RPC method.
- There isn't support for sending transactions _from_ a label, or for determining which label a transaction was sent from.
- Labels do not have a balance.

Here are the changes to RPC methods:

| Deprecated Method       | New Method            | Notes       |
| :---------------------- | :-------------------- | :-----------|
| `getaccount`            | `getaddressinfo`      | `getaddressinfo` returns a json object with address information instead of just the name of the account as a string. |
| `getaccountaddress`     | `getlabeladdress`     | `getlabeladdress` throws an error by default if the label does not already exist, but provides a `force` option for compatibility with existing applications. |
| `getaddressesbyaccount` | `getaddressesbylabel` | `getaddressesbylabel` returns a json object with the addresses as keys, instead of a list of strings. |
| `getreceivedbyaccount`  | `getreceivedbylabel`  | _no change in behavior_ |
| `listaccounts`          | `listlabels`          | `listlabels` does not return a balance or accept `minconf` and `watchonly` arguments. |
| `listreceivedbyaccount` | `listreceivedbylabel` | Both methods return new `label` fields, along with `account` fields for backward compatibility. |
| `move`                  | n/a                   | _no replacement_ |
| `sendfrom`              | n/a                   | _no replacement_ |
| `setaccount`            | `setlabel`            | Both methods now: <ul><li>allow assigning labels to any address, instead of raising an error if the address is not receiving address.<li>delete the previous label associated with an address when the final address using that label is reassigned to a different label, instead of making an implicit `getaccountaddress` call to ensure the previous label still has a receiving address. |

| Changed Method         | Notes   |
| :--------------------- | :------ |
| `getnewaddress`        | Renamed `account` named parameter to `label`. Still accepts `account` for backward compatibility. if running with '-deprecatedrpc=accounts' |
| `listunspent`          | Returns new `label` fields. `account` field will be returned for backward compatibility if running with '-deprecatedrpc=accounts' |
| `sendmany`             | The `account` named parameter has been renamed to `dummy`. If provided, the `dummy` parameter must be set to the empty string, unless running with the `-deprecatedrpc=accounts` argument (in which case functionality is unchanged). |
| `listtransactions`     | The `account` named parameter has been renamed to `dummy`. If provided, the `dummy` parameter must be set to the string `*`, unless running with the `-deprecatedrpc=accounts` argument (in which case functionality is unchanged). |
| `getbalance`           | `account`, `minconf` and `include_watchonly` parameters are deprecated, and can only be used if running with '-deprecatedrpc=accounts' |

 - `getlabeladdress` has been removed and replaced with `getaccountaddress`
   until v0.21 at which time `getaccountaddress` will also be removed.  To

Network
-------
 - When fetching a transaction announced by multiple peers, previous versions of
   Bitcoin ABC would sequentially attempt to download the transaction from each
   announcing peer until the transaction is received, in the order that those
   peers' announcements were received.  In this release, the download logic has
   changed to randomize the fetch order across peers and to prefer sending
   download requests to outbound peers over inbound peers. This fixes an issue
   where inbound peers can prevent a node from getting a transaction.

GUI changes
-----------
 - Block storage can be limited under Preferences, in the Main tab. Undoing
   this setting requires downloading the full blockchain again. This mode is
   incompatible with -txindex and -rescan.

Other
-----------
 - Fixed a bug with Multiwallets that have their own directories (i.e. cases
   such as `DATADIR/wallets/mywallet/wallet.dat`).  Backups of these wallets
   will now take each wallet's specific directory into account.
