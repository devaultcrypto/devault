#pragma once
#include <uint256.h>

/** A reference to a CKey: the Hash160 of its serialized public key */
class CKeyID : public uint160 {
public:
    CKeyID() : uint160() {}
    explicit CKeyID(const uint160 &in) : uint160(in) {}
};
/** BKey: the Hash160 of a serialized BLS public key */
class BKeyID : public uint160 {
public:
    BKeyID() : uint160() {}
    explicit BKeyID(const uint160 &in) : uint160(in) {}
};
