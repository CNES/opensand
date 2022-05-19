#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2020 TAS
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

# Author: Bastien TAURAN <bastien.tauran@viveris.fr>


LAN_IFACE=ens4
LAN_IP_GW=192.168.1.1
LAN_IP_WS_GW=192.168.1.2
LAN_NET_ST=192.168.2.0
NET_DIGITS=24

all: clean network

clean:
	ip link set $(LAN_IFACE) down

network: clean
	ip link set $(LAN_IFACE) up
	ip address flush dev $(LAN_IFACE)
	ip address replace $(LAN_IP_WS_GW)/$(NET_DIGITS) dev $(LAN_IFACE)
	ip route replace $(LAN_NET_ST)/$(NET_DIGITS) via $(LAN_IP_GW) dev $(LAN_IFACE)


.PHONY: all network clean
