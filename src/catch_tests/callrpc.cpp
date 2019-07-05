// Copyright (c) 2012-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "callrpc.h"
#include <rpc/client.h>
#include <rpc/server.h>

#include <config.h>
#include <utilsplitstring.h>
#include <univalue.h>

#include <catch2/catch.hpp>

UniValue CallRPC(std::string args) {
  std::vector<std::string> vArgs;
  Split(vArgs, args, " \t");
  std::string strMethod = vArgs[0];
  vArgs.erase(vArgs.begin());
  GlobalConfig config;
  JSONRPCRequest request;
  request.strMethod = strMethod;
  request.params = RPCConvertValues(strMethod, vArgs);
  request.fHelp = false;
  CHECK(tableRPC[strMethod]);
  try {
    UniValue result = tableRPC[strMethod]->call(config, request);
    return result;
  } catch (const UniValue &objError) { throw std::runtime_error(find_value(objError, "message").get_str()); }
}
