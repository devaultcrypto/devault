This release includes the following features and fixes since 1.0.6 release:

  * Separate null value (coin not used yet) from 0 value (spent)
  * Commit to db after writing MasterKey, make sure bdb-check checks for MasterKey
  * Add exec bdb-check with ability to decrypt wallet file
  * Add `consolidaterewards` RPC function. 
  * For Mac depends build, update clang version to 8.0.1 and remove some boost m4 files
  * Remove unnecessary copy Rewards functions in coincontrol
  * Calculate and show median reward amount
  * Improve Reward info in coincontrol
  * remove Unencryption state from EncryptionStatus and rename EncryptionStatus to WalletStatus

