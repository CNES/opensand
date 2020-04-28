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

# Author: Franklin SIMO <armelfrancklin.simotegueu@viveris.fr>


EMU_IFACE=ens4
EMU_IFACE_IP=192.168.18.1
LAN_IFACE=ens5
LAN_IFACE_IP=192.168.0.1
TAP_IFACE=opensand_tap
TAP_IFACE_MAC=00:00:00:00:00:02
BR_IFACE=opensand_br
BR_IFACE_IP=192.168.63.1
NET_DIGITS=24
FORWARDING=1
GW_LAN_IP=192.168.42.0/24
GW_LAN_IFACE_IP=192.168.63.254
GW_BR_IFACE_IP=192.168.63.254
ST_ID=1
CONF_PATH=/etc/opensand_ip/
COLLECTOR_IP=172.20.34.82

all: clean network run
	

clean:
	-ip address del $(EMU_IFACE_IP)/$(NET_DIGITS) dev $(EMU_IFACE)
	-ip link set dev $(TAP_IFACE) nomaster
	-ip address del $(BR_IFACE_IP)/$(NET_DIGITS) dev $(BR_IFACE)
	-ip address del $(LAN_IFACE_IP)/$(NET_DIGITS) dev $(LAN_IFACE)
	-ip link del $(TAP_IFACE)
	-ip link del $(BR_IFACE)
	-ip link set $(EMU_IFACE) down
	-ip link set $(LAN_IFACE) down

network:
	ip address add $(EMU_IFACE_IP)/$(NET_DIGITS) dev $(EMU_IFACE)
	ip tuntap add mode tap $(TAP_IFACE)
	ip link set dev $(TAP_IFACE) address $(TAP_IFACE_MAC)
	ip link add name $(BR_IFACE) type bridge
	ip address add $(BR_IFACE_IP)/$(NET_DIGITS) dev $(BR_IFACE)
	ip link set dev $(TAP_IFACE) master $(BR_IFACE)
	brctl setageing $(BR_IFACE) 0
	ip address add $(LAN_IFACE_IP)/$(NET_DIGITS) dev $(LAN_IFACE)
	ip link set $(EMU_IFACE) up
	ip link set $(BR_IFACE) up
	ip link set $(TAP_IFACE) up
	ip link set $(LAN_IFACE) up
	sysctl -w net.ipv4.conf.$(EMU_IFACE).forwarding=$(FORWARDING)
	sysctl -w net.ipv4.conf.$(BR_IFACE).forwarding=$(FORWARDING)
	sysctl -w net.ipv4.conf.$(LAN_IFACE).forwarding=$(FORWARDING)
	sysctl -w net.ipv4.ip_forward=$(FORWARDING)
	ip route add $(GW_LAN_IP) via $(GW_BR_IFACE_IP)

run:
	opensand-st -i $(ST_ID) -a $(EMU_IFACE_IP) -t $(TAP_IFACE) -c $(CONF_PATH) -f /var/log/opensand/ -r $(COLLECTOR_IP) -l 5362 -s 5361 &

stop:
	-pkill  opensand-st


.PHONY: all run network clean stop
