#!/bin/bash
ip route del 192.168.100.0/24 src 192.168.100.20 dev tap1
ip route add 192.168.100.0/24 via 192.168.100.1 dev tap0
ip route add 192.168.200.0/24 via 192.168.100.1 dev tap0
#arp -i tap0 -s 192.168.200.123 aa:bb:cc:00:01:02

touch auxresolv.conf
cat /etc/resolv.conf > auxresolv.conf
echo "nameserver 192.168.100.200" > /etc/resolv.conf
cat auxresolv.conf >> /etc/resolv.conf
rm auxresolv.conf
