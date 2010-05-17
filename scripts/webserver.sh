#!/bin/bash

bin/bridge tests/web/bridgeserver &
sleep 2

ip link set tap0 up
ip address add 192.168.200.123 dev tap0
