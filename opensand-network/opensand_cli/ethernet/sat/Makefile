EMU_IFACE=ens4
EMU_IFACE_IP=192.168.18.15
NET_DIGITS=24
CONF_PATH=/etc/opensand_eth/
COLLECTOR_IP=172.20.34.82



all: clean network run 

clean:
	-ip address del $(EMU_IFACE_IP)/$(NET_DIGITS) dev $(EMU_IFACE)
	-ip link set $(EMU_IFACE) down

network:  
	ip address add $(EMU_IFACE_IP)/$(NET_DIGITS) dev $(EMU_IFACE)
	ip link set $(EMU_IFACE) up
        
run:
	opensand-sat -a $(EMU_IFACE_IP) -c $(CONF_PATH) -r $(COLLECTOR_IP) -l 5362 -s 5361 &

stop:
	-pkill  opensand-sat

.PHONY: all run network clean stop

