// Copyright (c) 2012-2016 The Bitcoin Core developers
// Copyright (c) 2019 DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

/**
 * network protocol versioning
 */
static const int PROTOCOL_VERSION = 70000;

//! initial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 209;

//! Used for many things set for initial version
static const int LAUNCH_VERSION = 70000;

//! In this version, 'getheaders' was introduced.

//! disconnect from peers older than this proto version
static const int MIN_PEER_PROTO_VERSION = LAUNCH_VERSION;

// Preserve Old comments for old version names
//! nTime field added to CAddress, starting with this version;
//! if possible, avoid requesting addresses nodes older than this
//! BIP 0031, pong message, is enabled for all versions AFTER this one
//! "mempool" command, enhanced "getdata" behavior starts with this version
//! "filter*" commands are disabled without NODE_BLOOM after and including this version
//! "sendheaders" command and announcing blocks with headers starts with this version
//! "feefilter" tells peers to filter invs to you by fee starts with this version
//! short-id-based block download starts with this version
//! not banning for invalid compact blocks starts with this version

