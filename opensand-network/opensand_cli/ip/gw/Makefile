EMU_IFACE=ens4
EMU_IFACE_IP=192.168.18.42
LAN_IFACE=ens5
LAN_IFACE_IP=192.168.42.1
TAP_IFACE=opensand_tap
TAP_IFACE_MAC=00:00:00:00:00:01
BR_IFACE=opensand_br
BR_IFACE_IP=192.168.63.254
NET_DIGITS=24
FORWARDING=1
ST_LAN_IP=192.168.1.0/24
ST_BR_IFACE_IP=192.168.63.1
GW_ID=0
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
	ip route add $(ST_LAN_IP) via $(ST_BR_IFACE_IP)

run:
	opensand-gw -i $(GW_ID) -a $(EMU_IFACE_IP) -t $(TAP_IFACE) -c $(CONF_PATH) -f /var/log/opensand/ -r $(COLLECTOR_IP) -l 5362 -s 5361 &

stop:
	-pkill  opensand-gw


.PHONY: all run network clean stop
