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
LAN_IP_WS_ST=192.168.1.3
NET_DIGITS=24
VLAN_USED=10 20
VLAN_USED=10 20
VLAN_IP_BASE=192.168
VLAN_IP_TERM=3

all: clean network

clean:
	ip address flush dev $(LAN_IFACE)
	ip link set $(LAN_IFACE) down
	@for VLAN in $(VLAN_USED) ; do \
		ip address flush dev $(LAN_IFACE).$$VLAN up ; \
		ip link set $(LAN_IFACE).$$VLAN up down ; \
		ip link del link $(LAN_IFACE) name $(LAN_IFACE).$$VLAN type vlan id $$VLAN || true ; \
	done

network: clean
	ip address add $(LAN_IP_WS_ST)/$(NET_DIGITS) dev $(LAN_IFACE)
	ip link set $(LAN_IFACE) up
	@for VLAN in $(VLAN_USED) ; do \
		ip link add link $(LAN_IFACE) name $(LAN_IFACE).$$VLAN type vlan id $$VLAN ; \
		ip address add $(VLAN_IP_BASE).$$VLAN.$(VLAN_IP_TERM)/$(NET_DIGITS) dev $(LAN_IFACE).$$VLAN ; \
		ip link set $(LAN_IFACE).$$VLAN up ; \
	done


.PHONY: all network clean
