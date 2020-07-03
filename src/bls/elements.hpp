// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_BLSELEMENTS_HPP_
#define SRC_BLSELEMENTS_HPP_

#include "relic_conf.h"

#include <gmp.h>

namespace bls {
class G1Element;
class G2Element;
class GTElement;

class G1Element {
public:
    static const size_t SIZE = 48;
    static G1Element FromBytes(const uint8_t *bytes);
    static G1Element FromNative(const g1_t *element);
    static G1Element Generator();
    static G1Element FromMessage(
        const std::vector<uint8_t> &message,
        const uint8_t *dst,
        int dst_len);
    static G1Element FromMessageHash(
        const std::vector<uint8_t> &messageHash,
        const uint8_t *dst,
        int dst_len);

    g1_t p;
    G1Element();  // unity
    G1Element(const G1Element &element);
    friend bool operator==(G1Element const &a, G1Element const &b);
    friend bool operator!=(G1Element const &a, G1Element const &b);
    friend std::ostream &operator<<(std::ostream &os, G1Element const &s);
    friend G1Element &operator+=(G1Element &a, G1Element &b);
    friend G1Element operator+(G1Element &a, G1Element &b);
    friend G1Element &operator*=(G1Element &a, bn_t &k);
    friend G1Element operator*(G1Element &a, bn_t &k);
    friend G1Element operator*(bn_t &k, G1Element &a);

    GTElement pair(G2Element &b);
    friend GTElement operator&(G1Element &a, G2Element &b);

    G1Element Inverse();
    void Serialize(uint8_t *buffer) const;
    std::vector<uint8_t> Serialize() const;
    uint32_t GetFingerprint() const;

private:
    G1Element Exp(const bn_t n) const;
    static void CompressPoint(uint8_t *result, const g1_t *point);
};

class G2Element {
public:
    static const size_t SIZE = 96;
    static G2Element FromBytes(const uint8_t *data);
    static G2Element FromNative(const g2_t *element);
    static G2Element Generator();
    static G2Element FromMessage(
        const std::vector<uint8_t> &message,
        const uint8_t *dst,
        int dst_len);
    static G2Element FromMessageHash(
        const std::vector<uint8_t> &messageHash,
        const uint8_t *dst,
        int dst_len);

    g2_t q;
    G2Element();  // unity
    G2Element(const G2Element &element);
    void Serialize(uint8_t *buffer) const;
    std::vector<uint8_t> Serialize() const;

    friend bool operator==(G2Element const &a, G2Element const &b);
    friend bool operator!=(G2Element const &a, G2Element const &b);
    friend std::ostream &operator<<(std::ostream &os, G2Element const &s);
    friend G2Element &operator+=(G2Element &a, G2Element &b);
    friend G2Element operator+(G2Element &a, G2Element &b);
    friend G2Element &operator*=(G2Element &a, bn_t &k);
    friend G2Element operator*(G2Element &a, bn_t &k);
    friend G2Element operator*(bn_t &k, G2Element &a);

    GTElement pair(G1Element &a);
    // friend GTElement operator&(G1Element &a, G2Element &b);
    G2Element &operator=(const G2Element &rhs);

private:
    G2Element Exp(const bn_t n) const;
    static void CompressPoint(uint8_t *result, const g2_t *point);
};

class GTElement {
public:
    static const size_t SIZE = 384;
    static GTElement FromBytes(const uint8_t *data);
    static GTElement FromNative(const gt_t *element);
    GTElement(const GTElement &element);

    gt_t r;
    GTElement();  // unity

    void Serialize(uint8_t *buffer) const;
    std::vector<uint8_t> Serialize() const;

    friend bool operator==(GTElement const &a, GTElement const &b);
    friend bool operator!=(GTElement const &a, GTElement const &b);
    friend std::ostream &operator<<(std::ostream &os, GTElement const &s);
    GTElement &operator=(const GTElement &rhs);

private:
    static void CompressPoint(uint8_t *result, const gt_t *point);
};
}  // end namespace bls

#endif  // SRC_BLSELEMENTS_HPP_
