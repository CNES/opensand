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
#         Bastien TAURAN <bastien.tauran@viveris.fr>


GW_ID=0
ST_ID=1
SAT_ID=2
EMU_IFACE=ens4
EMU_IP_GW=192.168.18.42
EMU_IP_SAT=192.168.18.1
EMU_IP_ST=192.168.18.15
ST_LAN_NET=192.168.2.0/24
LAN_IFACE=ens5
LAN_IP=192.168.1.1
TAP_IFACE=opensand_tap
TAP_MAC_GW=00:00:00:00:00:01
TAP_MAC_ST=00:00:00:00:00:02
BR_IFACE=opensand_br
BR_IFACE_IP_GW=192.168.63.254
BR_IFACE_IP_ST=192.168.63.15
NET_DIGITS=24
FORWARDING=1
ENABLE_COLLECTOR=true
COLLECTOR_IP=10.10.0.142
ENABLE_LOCAL_LOGS=false
LOG_FOLDER=/var/log/opensand

all: clean network run

clean: stop
	ip link del $(TAP_IFACE) || true
	ip link del $(BR_IFACE) || true
	ip link set $(EMU_IFACE) down
	ip link set $(LAN_IFACE) down

network: clean
	ip link set $(EMU_IFACE) up
	ip link set $(LAN_IFACE) up
	sleep 0.1
	ip address flush dev $(EMU_IFACE)
	ip address flush dev $(LAN_IFACE)
	ip address replace $(LAN_IP)/$(NET_DIGITS) dev $(LAN_IFACE)
	ip address replace $(EMU_IP_GW)/$(NET_DIGITS) dev $(EMU_IFACE)
	ip tuntap add mode tap $(TAP_IFACE)
	ip link set dev $(TAP_IFACE) address $(TAP_MAC_GW)
	ip link add name $(BR_IFACE) type bridge
	ip address add $(BR_IFACE_IP_GW)/$(NET_DIGITS) dev $(BR_IFACE)
	ip link set dev $(TAP_IFACE) master $(BR_IFACE)
	ip link set $(BR_IFACE) up
	ip link set $(TAP_IFACE) up
	sysctl -w net.ipv4.conf.$(EMU_IFACE).forwarding=$(FORWARDING)
	sysctl -w net.ipv4.conf.$(BR_IFACE).forwarding=$(FORWARDING)
	sysctl -w net.ipv4.conf.$(LAN_IFACE).forwarding=$(FORWARDING)
	sysctl -w net.ipv4.ip_forward=$(FORWARDING)
	ip route replace $(ST_LAN_NET) via $(BR_IFACE_IP_ST) dev $(BR_IFACE)
	ip neighbor replace to $(BR_IFACE_IP_ST) dev $(BR_IFACE) lladdr $(TAP_MAC_ST)

generate-xml:
	cp infrastructure.xml infrastructure_updated.xml
	sed -i "s|GW_ID|$(GW_ID)|g" infrastructure_updated.xml
	sed -i "s|ST_ID|$(ST_ID)|g" infrastructure_updated.xml
	sed -i "s|SAT_ID|$(SAT_ID)|g" infrastructure_updated.xml
	sed -i "s|EMU_IP_GW|$(EMU_IP_GW)|g" infrastructure_updated.xml
	sed -i "s|EMU_IP_SAT|$(EMU_IP_SAT)|g" infrastructure_updated.xml
	sed -i "s|EMU_IP_ST|$(EMU_IP_ST)|g" infrastructure_updated.xml
	sed -i "s|TAP_IFACE|$(TAP_IFACE)|g" infrastructure_updated.xml
	sed -i "s|TAP_MAC_GW|$(TAP_MAC_GW)|g" infrastructure_updated.xml
	sed -i "s|TAP_MAC_ST|$(TAP_MAC_ST)|g" infrastructure_updated.xml
	sed -i "s|ENABLE_COLLECTOR|$(ENABLE_COLLECTOR)|g" infrastructure_updated.xml
	sed -i "s|COLLECTOR_IP|$(COLLECTOR_IP)|g" infrastructure_updated.xml
	sed -i "s|ENABLE_LOCAL_LOGS|$(ENABLE_LOCAL_LOGS)|g" infrastructure_updated.xml
	sed -i "s|LOG_FOLDER|$(LOG_FOLDER)|g" infrastructure_updated.xml
	cp topology.xml topology_updated.xml
	sed -i "s|GW_ID|$(GW_ID)|g" topology_updated.xml
	sed -i "s|SAT_ID|$(SAT_ID)|g" topology_updated.xml

run: generate-xml
	opensand -i infrastructure_updated.xml -t topology_updated.xml -p profile.xml &

stop:
	killall -q opensand || true


.PHONY: all run network clean stop generate-xml
