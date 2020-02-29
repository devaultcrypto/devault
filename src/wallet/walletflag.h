// Copyright (c) 2020 Jon Spock
// Copyright (c) 2020 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <serialize.h>

class WalletFlag {
  bool allow_private_keys{true};
  bool blank{false};
  bool has_bls{false};
  bool has_legacy{true}; // Later will make default false

  public:
  void SetBlank() { blank = true; }
  void SetPrivate() { allow_private_keys = true; }
  void SetBLS() { has_bls = true; }
  void SetLEGACY() { has_legacy = true; }

  void UnsetBlank() { blank = false; }
  void UnsetLEGACY() { has_legacy = false; }
  void UnsetPrivate() { allow_private_keys = false; }

  bool HasBLS() const { return has_bls; }
  bool HasLEGACY() const { return has_legacy; }
  bool GetBlank() const { return blank; }
  bool GetPrivate() const { return allow_private_keys; }

  // For Wallet before this class was added
  void SetLegacyWallet() {
      blank = false;
      allow_private_keys = true;
      has_bls = false;
      has_legacy = true;
  }
    
  ADD_SERIALIZE_METHODS;

  template <typename Stream, typename Operation> inline void SerializationOp(Stream &s, Operation ser_action) {
    READWRITE(allow_private_keys);
    READWRITE(blank);
    READWRITE(has_legacy);
    READWRITE(has_bls);
  }
};
