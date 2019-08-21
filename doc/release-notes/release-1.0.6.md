This release includes the following features and fixes since 1.0.5 release:

  * Add Menu items for Unlocking Wallet and Revealing Word phrase
  * Seed Words / Start-Up UI Overhaul
  * Enable menu item to reveal word phrase even if wallet is locked. If used when locked, ask for password first.
  * Relock wallet after revealing word phrase
  * Shrink height for reveal phrase window
  * Run clang-format on newer qt start files
  * Fix warnings on MacOS compile
  * added cmake stuff and pragma
  * Added mnemonic check with drag and drop stuff
  * Add filtering options for transactions to show Rewards/Budget separately
  * Upgrade MacOS version stuff for depends builds
  * Show number of unique addresses with viable rewards in getrewardinfo
  * Fix time calc for std::filesystem debug log renaming
  * more QT connect usage modernization (#179)
  * Move ValueFromAmount function, add new rpc command getdifficulties
  * Update more deprecated code for QT (#177)
  * Use std::filesystem on Catalina
  * Add new QT shortcuts for Sign / Verify Message (Ctrl+Shift+M & Ctrl+Shift+V)
  * Add direct RPC Console menu options in new sub-menu "Tools"
  * Re-design of buttons & radio widgets

Upstream Bitcoin-ABC/Bitcoin updates

* Merge #11480: [ui] Add toggle for unblinding password fields
