"""
List af available rules:
- dscp
- tos
- ip_source
- ip_destination
- port_source (TCP or UDP)
- port_destination (TCP or UDP)
- icmp_type (ICMP)
"""

import yaml
import subprocess

def check_ip(ip):
	ip = str(ip)
	if '/' not in ip:
		ip += "/32"
	addr, mask = ip.split('/')
	try:
		mask = int(mask)
	except ValueError:
		return False
	if not 0 < mask <= 32:
		return False
	elts = addr.split('.')
	if len(elts) != 4:
		return False
	for elt in elts:
		try:
			elt = int(elt)
		except ValueError:
			return False
		if not 0 <= elt < 256:
			return False
	return True

def check_port(port):
	try:
		port = int(port)
	except ValueError:
		return False
	return 0 <= port < 65536

def check_tos(tos):
	try:
		tos = int(tos)
	except ValueError:
		return False
	return 0 <= tos < 256

def check_dscp(dscp):
	try:
		dscp = int(dscp)
	except ValueError:
		return False
	return 0 <= dscp < 64

def check_icmp(icmp):
	try:
		icmp = int(icmp)
	except ValueError:
		return False
	return 0 <= icmp < 256

stream = open("pcp_rules.yml", 'r')
d = yaml.load(stream, Loader=yaml.FullLoader) or {}
commands = []


for pcp, rules in d.items():
	for rule in rules:
		command = []
		command_tcpudp = []
		command_icmp = []
		if "dscp" in rule and check_dscp(rule["dscp"]):
			command.append("-m dscp --dscp " + str(rule["dscp"]))
		if "tos" in rule and check_tos(rule["tos"]):
			command.append("-m tos --tos " + str(rule["tos"]))
		if "sport" in rule and check_port(rule["sport"]):
			command_tcpudp.append("--sport " + str(rule["sport"]))
		if "dport" in rule and check_port(rule["dport"]):
			command_tcpudp.append("--dport " + str(rule["dport"]))
		if "saddr" in rule and check_ip(rule["saddr"]):
			command.append("-s " + rule["saddr"])
		if "daddr" in rule and check_ip(rule["daddr"]):
			command.append("-d " + rule["daddr"])
		if "icmp_type" in rule and check_icmp(rule["icmp_type"]):
			command_icmp.append("--icmp-type " + str(rule["icmp_type"]))
		if command_tcpudp:
			commands.append("iptables -t mangle -A POSTROUTING " + " ".join(command) + " -p tcp " + " ".join(command_tcpudp) + " -j CLASSIFY --set-class 0:" + str(pcp+1))
			commands.append("iptables -t mangle -A POSTROUTING " + " ".join(command) + " -p udp " + " ".join(command_tcpudp) + " -j CLASSIFY --set-class 0:" + str(pcp+1))
		if command_icmp:
			commands.append("iptables -t mangle -A POSTROUTING " + " ".join(command) + " -p icmp " + " ".join(command_icmp) + " -j CLASSIFY --set-class 0:" + str(pcp+1))
		if not command_tcpudp and not command_icmp:
			commands.append("iptables -t mangle -A POSTROUTING " + " ".join(command) + " -j CLASSIFY --set-class 0:" + str(pcp+1))

stream.close ()

subprocess.run("iptables -t filter -F".split())
subprocess.run("iptables -t mangle -F".split())
subprocess.run("iptables -t nat -F".split())

subprocess.run("iptables -t mangle -A FORWARD -j CLASSIFY --set-class 0:1".split())

for c in commands:
	subprocess.run(c.split())
