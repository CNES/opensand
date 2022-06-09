# OpenSAND network config with netns

## Configure the entities

Modify the file `opensand_netns.py` with the IDs of the entities you want to configure.

## Create the net namespaces

```bash
sudo ./opensand_network.py up
```

The netns names will be `gw1` for gateway with ID 1, `st2` for terminal with ID 2, and `sat3` for satellite with ID 3, etc.

The script creates an emu subnetwork 192.168.18.0/24, and a dev subnetwork 192.168.100.0/24.

The last byte of each IP address is the ID of each entity.

## Run OpenSAND

```bash
# I needed that to make it work, not sure why
export LD_LIBRARY_PATH="/usr/local/lib:/usr/local/lib/opensand/plugins"

sudo ip netns exec {netns_name} env LD_LIBRARY_PATH=$LD_LIBRARY_PATH opensand -v -i infra.xml -t topo.xml -p profile.xml
```

## Delete the net namespaces

```bash
sudo ./opensand_network.py down
```
