#!/bin/sh

DIST_NET="192.168.19.0/24"
LOCAL_NET="192.168.20.0/24"
ifconfig opensand_tun up
ip route del $LOCAL_NET dev opensand_tun
ip route add $DIST_NET dev opensand_tun
