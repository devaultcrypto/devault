#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Copyright (c) 2019 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
This checks if all command line args are documented.
Return value is 0 to indicate no error.

Author: @MarcoFalke
'''

from subprocess import check_output
from pprint import PrettyPrinter
import re

FOLDER_SRC = 'src'
FOLDER_TEST = 'test'
PATH_SRC = '`git rev-parse --show-toplevel`/{}'.format(FOLDER_SRC)

REGEX_ARG = '(?:ForceSet|SoftSet|Get|Is)(?:Bool)?Args?(?:Set)?\(\s*"(-[^"]+)"'
REGEX_DOC = 'HelpMessageOpt\(\s*"(-[^"=]+?)(?:=|")'

CMD_GREP_ARGS_SRC = r"grep -rIPzo '{}' {} --exclude-dir={}".format(
    REGEX_ARG, PATH_SRC, FOLDER_TEST)
CMD_GREP_DOCS = r"grep -rIPzo '{}' {}".format(REGEX_DOC, PATH_SRC)

# list unsupported, deprecated and duplicate args as they need no documentation
SET_DOC_OPTIONAL = set(['-benchmark',
                        '-blockminsize',
                        '-dbcrashratio',
                        '-debugnet',
                        '-forcecompactdb',
                        # TODO remove after the may 2019 fork
                        '-greatwallactivationtime',
                        '-h',
                        '-help',
                        '-parkdeepreorg',
                        '-promiscuousmempoolflags',
                        '-replayprotectionactivationtime',
                        '-rpcssl',
                        '-socks',
                        '-tor',
                        '-whitelistalwaysrelay'])

# list false positive unknows arguments
SET_FALSE_POSITIVE_UNKNOWNS = set(['-nodebug',
                                   '-zmqpubhashblock',
                                   '-zmqpubhashtx',
                                   '-zmqpubrawblock',
                                   '-zmqpubrawtx'])


def main():
    used = check_output(CMD_GREP_ARGS_SRC, shell=True).decode()
    docd = check_output(CMD_GREP_DOCS, shell=True).decode()

    args_used = set(re.findall(REGEX_ARG, used))
    args_used |= SET_FALSE_POSITIVE_UNKNOWNS
    args_docd = set(re.findall(REGEX_DOC, docd))
    args_need_doc = args_used - args_docd - SET_DOC_OPTIONAL
    args_unknown = args_docd - args_used

    pp = PrettyPrinter()
    print("Args used        : {}".format(len(args_used)))
    print("Args documented  : {}".format(len(args_docd)))
    print("Args undocumented: {} ({} don't need documentation)".format(
        len(args_need_doc), len(SET_DOC_OPTIONAL)))
    pp.pprint(args_need_doc)
    print("Args unknown     : {}".format(len(args_unknown)))
    pp.pprint(args_unknown)


if __name__ == "__main__":
    main()
