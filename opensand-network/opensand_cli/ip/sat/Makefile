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
EMU_IFACE_IP=192.168.18.15
NET_DIGITS=24
CONF_PATH=/etc/opensand_ip/
COLLECTOR_IP=172.20.34.82


all: clean network run 

clean:
	-ip address del $(EMU_IFACE_IP)/$(NET_DIGITS) dev $(EMU_IFACE)
	-ip link set $(EMU_IFACE) down

network:  
	ip address add $(EMU_IFACE_IP)/$(NET_DIGITS) dev $(EMU_IFACE)
	ip link set $(EMU_IFACE) up
        
run:
	opensand-sat -a $(EMU_IFACE_IP) -c $(CONF_PATH) -r $(COLLECOR_IP) -l 5362 -s 5361 &

stop:
	-pkill  opensand-sat

.PHONY: all run network clean stop

