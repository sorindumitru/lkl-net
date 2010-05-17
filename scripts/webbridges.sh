#!/bin/bash

# TODO: check if user is root

bin/bridge tests/web/bridgebrowser &
sleep 1
#bin/bridge tests/web/bridgedns &
#sleep 1
bin/bridge tests/web/bridgerouter &
#sleep 1
#bin/bridge tests/web/bridgeserver &
sleep 2

ip link set tap0 up
ip address add  192.168.100.123 dev tap0
#ip link set tap1 up
#ip address add 192.168.100.200 dev tap1
ip link set tap1 up
ip address add 192.168.100.20 dev tap1
#ip link set tap3 up
#ip address add 192.168.200.123 dev tap3
