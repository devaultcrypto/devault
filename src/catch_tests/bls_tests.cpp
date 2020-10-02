// Copyright (c) 2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <catch_tests/test_bitcoin.h>
#include <catch_tests/lcg.h>

#include <script/interpreter.h>

#include "catch_unit.h"

#include <array>
#include <bitset>

typedef std::vector<uint8_t> valtype;
typedef std::vector<valtype> stacktype;

// BOOST_FIXTURE_TEST_SUITE(schnorr_tests, BasicTestingSetup)

static valtype SignatureWithHashType(valtype vchSig, SigHashType sigHash) {
  vchSig.push_back(static_cast<uint8_t>(sigHash.getRawSigHashType()));
  return vchSig;
}

const uint8_t vchPrivkey[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

struct KeyData {
  CKey privkeyC;
  CPubKey pubkeyC;

  KeyData() {
    privkeyC.Set(vchPrivkey, vchPrivkey + 32);
    pubkeyC = privkeyC.GetPubKeyForBLS();
  }
};

static void CheckError(uint32_t flags, const stacktype &original_stack, const CScript &script, ScriptError expected) {
  BaseSignatureChecker sigchecker;
  ScriptError err = ScriptError::OK;
  stacktype stack{original_stack};
  bool r = EvalScript(stack, script, flags, sigchecker, &err);
  BOOST_CHECK(!r);
  BOOST_CHECK_EQUAL(err, expected);
}

static void CheckPass(uint32_t flags, const stacktype &original_stack, const CScript &script,
                      const stacktype &expected) {
  BaseSignatureChecker sigchecker;
  ScriptError err = ScriptError::OK;
  stacktype stack{original_stack};
  bool r = EvalScript(stack, script, flags, sigchecker, &err);
  BOOST_CHECK(r);
  BOOST_CHECK_EQUAL(err, ScriptError::OK);
  BOOST_CHECK(stack == expected);
}

TEST_CASE("opcodes_random_flags") {
  BasicTestingSetup setup;
  // Test script execution of the six signature opcodes with Schnorr-sized
  // signatures, and probe failure mode under a very wide variety of flags.

  // A counterpart to this can be found in sigencoding_tests.cpp, which only
  // probes the sig encoding functions.

  // Grab the various pubkey types.
  KeyData kd;
  valtype pubkeyC = ToByteVector(kd.pubkeyC);

  // Script endings. The non-verify variants will complete OK and the verify
  // variant will complete with ScriptError::<opcodename>, that is, unless
  // there is a flag-dependent error which we will be testing for.
  const CScript scriptCHECKSIG = CScript() << OP_CHECKSIG << OP_NOT << OP_VERIFY;
  const CScript scriptCHECKSIGVERIFY = CScript() << OP_CHECKSIGVERIFY;
  const CScript scriptCHECKDATASIG = CScript() << OP_CHECKDATASIG << OP_NOT << OP_VERIFY;
  const CScript scriptCHECKDATASIGVERIFY = CScript() << OP_CHECKDATASIGVERIFY;
  const CScript scriptCHECKMULTISIG = CScript() << OP_CHECKMULTISIG << OP_NOT << OP_VERIFY;
  const CScript scriptCHECKMULTISIGVERIFY = CScript() << OP_CHECKMULTISIGVERIFY;

  // all-zero signature: valid encoding for BLS? but invalid for DER.
  valtype Zero96{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  // this is a validly-encoded ? byte sig; also a valid BLS? encoding.
  valtype DER96{0x30, 0x3e, 0x02, 0x1d, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
                0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
                0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
                0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
                0x44, 0x02, 0x1d, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
                0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44};

    uint32_t flags = SCRIPT_ENABLE_SIGHASH_FORKID | SCRIPT_ENABLE_BLS | SCRIPT_VERIFY_STRICTENC; // for now | SCRIPT_VERIFY_NULLFAIL;

    
  const bool hasForkId = true;
  const bool hasBLS = true;

  // Prepare transaction sigs with right hashtype byte.
  valtype DER96_with_hashtype = SignatureWithHashType(DER96, SigHashType().withForkId(hasForkId));
  valtype Zero96_with_hashtype = SignatureWithHashType(Zero96, SigHashType().withForkId(hasForkId));

  // Test CHECKSIG & CHECKDATASIG with Zero sig, which can fail from encoding, otherwise upon verification.
  CheckPass(flags, {Zero96_with_hashtype, pubkeyC}, scriptCHECKSIG, {});
  CheckError(flags, {Zero96_with_hashtype, pubkeyC}, scriptCHECKSIGVERIFY, ScriptError::CHECKSIGVERIFY);
  //CheckPass(flags, {Zero96, {}, pubkeyC}, scriptCHECKDATASIG, {});
  //CheckError(flags, {Zero96, {}, pubkeyC}, scriptCHECKDATASIGVERIFY, ScriptError::CHECKDATASIGVERIFY);

  // Test CHECKSIG & CHECKDATASIG with DER sig, which fails upon verification.
  CheckPass(flags, {DER96_with_hashtype, pubkeyC}, scriptCHECKSIG, {});
  CheckError(flags, {DER96_with_hashtype, pubkeyC}, scriptCHECKSIGVERIFY, ScriptError::CHECKSIGVERIFY);
  //CheckError(flags, {DER96, {}, pubkeyC}, scriptCHECKDATASIG, {});
  //CheckError(flags, {DER96, {}, pubkeyC}, scriptCHECKDATASIGVERIFY, ScriptError::CHECKDATASIGVERIFY);

  // test OP_CHECKMULTISIG/VERIFY
  CheckError(flags, {{}, Zero96_with_hashtype, {1}, pubkeyC, {1}}, scriptCHECKMULTISIG, ScriptError::SIG_BADLENGTH);
  CheckError(flags, {{}, Zero96_with_hashtype, {1}, pubkeyC, {1}}, scriptCHECKMULTISIGVERIFY,
             ScriptError::SIG_BADLENGTH);
  CheckError(flags, {{}, DER96_with_hashtype, {1}, pubkeyC, {1}}, scriptCHECKMULTISIG, ScriptError::SIG_BADLENGTH);
  CheckError(flags, {{}, DER96_with_hashtype, {1}, pubkeyC, {1}}, scriptCHECKMULTISIGVERIFY,
             ScriptError::SIG_BADLENGTH);
}

// BOOST_AUTO_TEST_SUITE_END()
