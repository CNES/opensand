#!/bin/python3

import sys
from opensand_network_utils import *

dev_bridge = "dev_bridge"
emu_bridge = "emu_bridge"

# IDs of the entities to configure
gateways = [1, 2]
terminals = [11, 12]
satellites = [21, 22]


def create_entity(type, id, create_tap):
    name = f"{type}{id}"
    create_netns(name)
    set_up("lo", name)
    
    # dev network
    veth_dev_remote = f"dev_{name}"
    veth_dev_local = f"dev_lo_{name}"
    create_veth_ifaces_pair(veth_dev_remote, veth_dev_local)
    move_iface(veth_dev_remote, name)
    add_address(veth_dev_remote, f"192.168.100.{id}/24", name)
    attach_iface(dev_bridge, veth_dev_local)
    set_up(veth_dev_local)
    set_up(veth_dev_remote, name)

    # emu network
    veth_emu_remote = f"emu_{name}"
    veth_emu_local = f"emu_lo_{name}"
    create_veth_ifaces_pair(veth_emu_remote, veth_emu_local)
    move_iface(veth_emu_remote, name)
    add_address(veth_emu_remote, f"192.168.18.{id}/24", name)
    attach_iface(emu_bridge, veth_emu_local)
    set_up(veth_emu_local)
    set_up(veth_emu_remote, name)
    
    if create_tap:
        tap_interface = "opensand_tap"
        create_tap_iface(tap_interface, name)
        set_mac_address(tap_interface, f"00:00:00:00:00:{id:0>2}", name)
        add_address(tap_interface, f"192.168.63.{id}/24", name)
        set_up(tap_interface, name)


def setup():
    create_bridge(dev_bridge, [])
    create_bridge(emu_bridge, [])
    for id in terminals:
        create_entity("st", id, True)
    for id in gateways:
        create_entity("gw", id, True)
    for id in satellites:
        create_entity("sat", id, False)
    
    add_address(dev_bridge, "192.168.100.100/24")
    set_up(dev_bridge)
    set_up(emu_bridge)

def clean():
    set_stop_on_error(False)
    for id in terminals:
        delete_netns(f"st{id}")
    for id in gateways:
        delete_netns(f"gw{id}")
    for id in satellites:
        delete_netns(f"sat{id}")

    delete_iface("dev_bridge")
    delete_iface("emu_bridge")
    set_stop_on_error(True)


def print_usage_and_exit(exit_code):
    print("Usage: opensand_network <up|down>")
    sys.exit(exit_code)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print_usage_and_exit(1)
    if sys.argv[1] == "up":
        setup()
    elif sys.argv[1] == "down":
        clean()
    else:
        print_usage_and_exit(2)
