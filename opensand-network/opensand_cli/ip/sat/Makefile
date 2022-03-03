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


EMU_IFACE=ens4
EMU_IFACE_IP=192.168.18.1
NET_DIGITS=24
CONF_PATH=/etc/opensand_ip/
COLLECTOR_IP=172.20.34.160

# TODO generate xml value for infrastructure.xml, and add path variable ?
# TODO clean: useful to del lan interface and set down ?
# TODO run: collector + logs ?
# TODO clean non used vars
# TODO do not print opensand logs on console


all: clean network run 

clean: stop
	ip address flush dev $(EMU_IFACE)
	ip link set $(EMU_IFACE) down

network: clean
	ip address add $(EMU_IFACE_IP)/$(NET_DIGITS) dev $(EMU_IFACE)
	ip link set $(EMU_IFACE) up

run:
	mkdir -p /var/log/opensand
	opensand -i infrastructure.xml -t topology.xml -v > /var/log/opensand/opensand_sat.log &

stop:
	killall -q opensand || true

.PHONY: all run network clean stop
.SILENT: run

