#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2014 TAS
# Copyright © 2014 CNES
#
#
# This file is part of the OpenSAND testbed.
#
#
# OpenSAND is free software : you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see http://www.gnu.org/licenses/.
#
#

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
opensand_manager_core/utils.py - Utilities for OpenSAND Manager
"""

import shutil
import os

OPENSAND_PATH = "/usr/share/opensand/"
COL_RED="\033[31m"
COL_GREEN="\033[32m"
COL_YELLOW="\033[33m"
COL_BLUE="\033[34m"
COL_BOLD="\033[1m"
COL_END="\033[0m"

MAX_DATA_LENGTH = 8192

# Host
GW  = "gw"
ST  = "st"
SAT = "sat"
WS  = "ws"

# Common
GLOBAL      = "global"
TOPOLOGY    = "topology"
COMMON      = "common"
SPOT        = "spot"
ID          = "id"
TAL_ID      = "tal_id"
CATEGORY    = "category"

# Band
FORWARD_DOWN      = "forward_down"
RETURN_UP         = "return_up"
FMT_GROUPS        = "fmt_groups"
CARRIERS_DISTRIB  = "carriers_distribution"
TAL_AFFECTATIONS  = "tal_affectations"
TAL_DEF_AFF       = "tal_default_affectation"
BANDWIDTH         = "bandwidth"
ROLL_OFF          = "roll_off"
FMT_ID            = "fmt_id"
FMT_GROUP         = "fmt_group"
SYMBOL_RATE       = "symbol_rate"
RATIO             = "ratio"
ACCESS_TYPE       = "access_type"
CCM               = "CCM"
ACM               = "ACM"
VCM               = "VCM"
DAMA              = "DAMA"
ALOHA             = "ALOHA"
SCPC              = "SCPC"

# Section
RETURN_UP_BAND    = "return_up_band"
FORWARD_DOWN_BAND = "forward_down_band"
PHYSICAL_LAYER    = "physical_layer"

GSE      = "GSE"
AAL5_ATM = "AAL5/ATM"

# Topology
CARRIERS      = "carriers"
TERMINALS     = "terminals"
CARRIER_TYPE  = "type"
DATA_OUT_GW   = "data_out_gw"
DATA_IN_GW    = "data_in_gw"
DATA_OUT_ST   = "data_out_st"
DATA_IN_ST    = "data_in_st"
LOGON_IN      = "logon_in"
LOGON_OUT     = "logon_out"
CTRL_IN       = "ctrl_in"
CTRL_OUT      = "ctrl_out"
IP_MULTICAST  = "ip_multicast"
IP_ADDRESS    = "ip_address"
TERMINAL_V4   = "terminal_v4"
TERMINAL_V6   = "terminal_v6"
TERMINAL_ETH  = "terminal_eth"
LAN_IPV4      = "lan_ipv4"
LAN_IPV6      = "lan_ipv6"

# Path
PATH_BAND     = "_band/"
PATH_IPV4     = "/configuration/sarp/ipv4"
PATH_IPV6     = "/configuration/sarp/ipv6"
PATH_ETERNET  = "/configuration/sarp/ethernet"
PATH_TERM_V4  = "/configuration/sarp/ipv4/terminal_v4"
PATH_TERM_V6  = "/configuration/sarp/ipv4/terminal_v6"
PATH_TERM_ETH = "/configuration/sarp/ipv4/terminal_eth"
PATH_DEFAULT_SPOT = "/configuration/spot_table/default_spot"
PATH_SPOT = "/configuration/spot_table"
PATH_DEFAULT_GW = "/configuration/gw_table/default_gw"
PATH_GW = "/configuration/gw_table"

def get_conf_xpath(element, link = '', spot_id = 0, gw_id = 1):
    path = ""
    spot = "spot/"
    if spot_id != 0:
        spot = "spot[@id='" + str(spot_id) + "']/"
        if gw_id != 1:
            spot = "spot[@id='" + str(spot_id) + "'and@gw='" + gw_id + "']/"
    
    if element == FMT_GROUPS or\
       element == CARRIERS_DISTRIB or\
       element == TAL_AFFECTATIONS or\
       element == TAL_DEF_AFF or\
       element == BANDWIDTH:
        path = '//' + link + PATH_BAND + spot + element
    else:
        if link == FORWARD_DOWN or link == RETURN_UP:
            path = '//' + link + PATH_BAND + element
        else:
            path = '//' + link + '/' + element
    return path


def _bold(msg):
    """ return the message with bold characters """
    return COL_BOLD + msg + COL_END

def _color(msg, color, bold):
    """ return the message colored """
    msg = color + msg + COL_END
    if bold:
        return _bold(msg)
    return msg

def red(msg, bold=False):
    """ return the message colored in red """
    return _color(msg, COL_RED, bold)

def green(msg, bold=False):
    """ return the message colored in green """
    return _color(msg, COL_GREEN, bold)

def blue(msg, bold=False):
    """ return the message colored in blue """
    return _color(msg, COL_BLUE, bold)

def yellow(msg, bold=False):
    """ return the message colored in yellow """
    return _color(msg, COL_YELLOW, bold)

def copytree(src, dst):
    """ Recursively copy a directory tree using copy2()
        Adapted from shutil.copytree """
    names = os.listdir(src)

    if not os.path.exists(dst):
        os.makedirs(dst)
    for name in names:
        srcname = os.path.join(src, name)
        dstname = os.path.join(dst, name)
        try:
            if os.path.isdir(srcname):
                copytree(srcname, dstname)
            else:
                shutil.copy2(srcname, dstname)
        except (IOError, os.error):
            raise
    try:
        shutil.copystat(src, dst)
    except OSError:
        raise


