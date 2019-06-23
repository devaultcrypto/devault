// Copyright 2018 The Beam Team
// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
//
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <wallet/mnemonic.h>
#include <crypto/pkcs5_pbkdf2.h>
#include <crypto/sha256.h>
#include <utilstrencodings.h>
#include <utilsplitstring.h>
#include <random.h>
#include <iostream>
#include <random>
#include <cassert>
#include <algorithm>

using namespace std;

namespace mnemonic {
namespace {
const size_t bitsPerWord = 11;
const uint8_t byteBits = 8;
const string passphrasePrefix = "mnemonic";
const size_t hmacIterations = 2048;
const size_t sizeHash = 512 >> 3;

uint8_t shiftBits(size_t bit) { return (1 << (byteBits - (bit % byteBits) - 1)); }
}

std::tuple<WordList, std::vector<uint8_t> > GenerateSeedPhrase(int nWords) {
  assert((nWords == 12) || (nWords == 24));
  // 1) Generate Random bytes (strong)
  // 2) Map to 12 or 24 words (with checksum word)
  // 3) Map back to 'seed' - 64 bits
  // 4) Use 1st 32 bytes as secret key for Master HD key
  // Using 2) we can re-generate 4) as needed
  int size = (nWords == 12) ? 16 : 32;
  std::vector<uint8_t> keydata(size);
  GetStrongRandBytes(keydata.data(), keydata.size());
  mnemonic::WordList words = mapBitsToMnemonic(keydata, language::en);
  //std::cout << "Words = " << join(words,",") << "\n";
  std::vector<uint8_t> hashWords = decodeMnemonic(words);
  return std::tuple(words, hashWords);
}

bool isValidSeedPhrase(const std::string& seedphrase) {
  mnemonic::WordList words;
  Split(words, seedphrase, " ");
  return isValidMnemonic(words, language::en);
} 

// std::variant may work better here in the future
std::tuple<bool, WordList, std::vector<uint8_t> > CheckSeedPhrase(const std::string& seedphrase) {
  // At this point we could have either used keydata or hashRet
  // for the master key, hashRet just adds mnemonic passphrase, etc
  std::vector<uint8_t> hashWords;
  mnemonic::WordList words;
  Split(words, seedphrase, " ");
  // Ignore CHECKSUM check!
  if (isValidMnemonic(words, language::en)) {
    hashWords = decodeMnemonic(words);
    return std::tuple(true, words, hashWords);
  }  
  return std::tuple(false, words, hashWords);
} 

WordList mapBitsToMnemonic(vector<uint8_t> &data, const Dictionary &dict) {
  // entropy should be 16 bytes or 128 bits for 12 words
  // or 32 bytes for 24 words
  assert((data.size() == 16) || (data.size() == 32));

  uint8_t checksum[32];
  CSHA256 chasher;
  chasher.Write(&data[0], data.size());
  chasher.Finalize(checksum);

  vector<string> words;
  size_t bit = 0;

  data.push_back(checksum[0]);
  int wordCount = (data.size() == 16) ? 12 : 24;

  for (int word = 0; word < wordCount; word++) {
    size_t position = 0;
    for (size_t loop = 0; loop < bitsPerWord; loop++) {
      bit = (word * bitsPerWord + loop);
      position <<= 1;

      const auto byte = bit / byteBits;

      if ((data[byte] & shiftBits(bit)) > 0) position++;
    }

    words.push_back(dict[position]);
  }

  return words;
}
// Matches bip39 seed on https://iancoleman.io/bip39/
vector<uint8_t> decodeMnemonic(const WordList &words) {
  const string sentence = join(words, " ");
  vector<uint8_t> passphrase(sentence.begin(), sentence.end());
  vector<uint8_t> salt(passphrasePrefix.begin(), passphrasePrefix.end());
  vector<uint8_t> hash(sizeHash);

  const auto result = pkcs5_pbkdf2(passphrase.data(), passphrase.size(), salt.data(), salt.size(), hash.data(),
                                   hash.size(), hmacIterations);

  if (result != 0) throw MnemonicException("pbkdf2 returned bad result");

  return hash;
}

bool isAllowedWord(const string &word, const Dictionary &dict) {
  assert(is_sorted(dict.begin(), dict.end()));
  return binary_search(dict.begin(), dict.end(), word);
}

bool isValidMnemonic(const WordList &words, const Dictionary &dict) {
  return ((words.size() == 12) || (words.size() == 24)) &&
         all_of(words.begin(), words.end(), [&dict](const string &w) { return isAllowedWord(w, dict); });
}
}
