This release includes the following features and fixes since 1.1.1 release:

This is a mandatory release by time of 6th Superblock

 - remove need for Should Connect which banned 1.0.7 or lower nodes
 - Change MIN_PEER_PROTO_VERSION to 70020 to ban older nodes
 - Cut-off rewards < 25k after 6th superblock
 - Restore any reward if needed (instead of ones > 25k)
 - When showing full transaction with dropdown menu, also show the relevant block it occurred on
 - Instead of showing Fully Synchronized on main window, show Synced at Block with block number
 - Fix bdb-check to be able to parse -testnet for testnet wallet checks (still only builds on MacOS)
 - Other code refactoring
 - Throw rpc error if verifychain fails, fix verifychain issues (bypass reward checks)
 - Cleanup of previous hard fork code
 - Add tex version of whitepaper
 - Cancel operations in validation when progress dialog is closed

