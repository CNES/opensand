import yaml
import subprocess

stream = open("pcp_rules.yml", 'r')
d = yaml.load(stream, Loader=yaml.FullLoader) or {}
commands = []


subprocess.run("iptables -t filter -F".split())
subprocess.run("iptables -t mangle -F".split())
subprocess.run("iptables -t nat -F".split())

subprocess.run("iptables -t mangle -A FORWARD -j CLASSIFY --set-class 0:1".split())


for i in range(8):
	pcp = "pcp"+str(i)
	fields = d[pcp]
	for x in fields["dscp"]:
		commands.append("iptables -t mangle -A POSTROUTING -m dscp --dscp " + str(x) + " -j CLASSIFY --set-class 0:" + str(i+1))
	for x in fields["tos"]:
		commands.append("iptables -t mangle -A POSTROUTING -m tos --tos " + str(x) + " -j CLASSIFY --set-class 0:" + str(i+1))
	for x in fields["sport"]:
		commands.append("iptables -t mangle -A POSTROUTING -p tcp --sport " + str(x) + " -j CLASSIFY --set-class 0:" + str(i+1))
		commands.append("iptables -t mangle -A POSTROUTING -p udp --sport " + str(x) + " -j CLASSIFY --set-class 0:" + str(i+1))
	for x in fields["dport"]:
		commands.append("iptables -t mangle -A POSTROUTING -p tcp --dport " + str(x) + " -j CLASSIFY --set-class 0:" + str(i+1))
		commands.append("iptables -t mangle -A POSTROUTING -p udp --dport " + str(x) + " -j CLASSIFY --set-class 0:" + str(i+1))
	for x in fields["saddr"]:
		commands.append("iptables -t mangle -A POSTROUTING -p tcp -s " + str(x) + " -j CLASSIFY --set-class 0:" + str(i+1))
		commands.append("iptables -t mangle -A POSTROUTING -p udp -s " + str(x) + " -j CLASSIFY --set-class 0:" + str(i+1))
	for x in fields["daddr"]:
		commands.append("iptables -t mangle -A POSTROUTING -p tcp -d " + str(x) + " -j CLASSIFY --set-class 0:" + str(i+1))
		commands.append("iptables -t mangle -A POSTROUTING -p udp -d " + str(x) + " -j CLASSIFY --set-class 0:" + str(i+1))
	for x in fields["icmp_type"]:
		commands.append("iptables -t mangle -A POSTROUTING -p icmp --icmp-type " + str(x) + " -j CLASSIFY --set-class 0:" + str(i+1))

stream.close ()

for c in commands:
	subprocess.run(c.split())
