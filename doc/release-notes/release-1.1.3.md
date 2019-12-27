This release includes the following features and fixes since 1.1.2 release:

 - CMake build changes
    Use std::filesystem if building with GCC > 9 or on Catalina
    Upgrades to allow cross compiles to work on Debian Buster
    Boost downgraded to 1.64 to allow cmake cross compiles to work
    Other Cmake cleanup/refactoring
    Minimum allow cmake version is 3.10
 - Move devault specific documentation to it's own directory
 - Dump ordered rewards to log file upon shutdown for debugability of issues
 - Fix -reindex for gensesis block
 - Change validation of Cold Rewards
    Now check if paid reward is a valid reward and amount, out of order payouts allowed
    Also allows skipping paying a cold reward without consensus issue

Upstream bitcoin-abc updates

 - `devault-qt -resetguisettings` now generates a backup of the former GUI settings.
 - Minor logging improvements.
 - Introduced `submitheader` RPC for submitting header candidates as chaintips.
