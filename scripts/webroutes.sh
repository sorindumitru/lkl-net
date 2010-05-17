#!/bin/bash

ip route add 192.168.100.0/24 via 192.168.100.123 dev tap0
ip route add 192.168.200.0/24 via 192.168.100.123 dev tap0