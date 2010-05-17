#!/bin/bash

# TODO: check if user is root

bin/bridge tests/web/bridgedns &
sleep 2

ip link set tap0 up
ip address add 192.168.100.200 dev tap0
ip route add 192.168.100.0/24 via 192.168.100.200 dev tap0