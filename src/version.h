// Copyright (c) 2012-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

/**
 * network protocol versioning
 */
static const int PROTOCOL_VERSION = 70015;

//! initial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 209;

//! disconnect from peers older than this proto version
static const int MIN_PEER_PROTO_VERSION = 70015; // Same as initial Protocol Version for start

//! Remove LEGACY BitcoinVersion stuff
//!
//! In this version, 'getheaders' was introduced.
//! nTime field added to CAddress, starting with this version;
//! if possible, avoid requesting addresses nodes older than this
//! BIP 0031, pong message, is enabled for all versions AFTER this one
//! "mempool" command, enhanced "getdata" behavior starts with this version
//! "filter*" commands are disabled without NODE_BLOOM after and including this version
//! "sendheaders" command and announcing blocks with headers starts with this version
//! "feefilter" tells peers to filter invs to you by fee starts with this version
//! short-id-based block download starts with this version
//! not banning for invalid compact blocks starts with this version


