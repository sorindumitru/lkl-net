#!/bin/bash

bin/bridge tests/web/bridgeserver &
sleep 2

ip link set tap0 up
ip address add 192.168.200.123/24 dev tap0
ip route add 192.168.100.0/24 via 192.168.200.123 dev tap0