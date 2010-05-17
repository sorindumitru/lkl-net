#!/bin/bash

ip route add 192.168.100.0/24 dev tap0
ip route add 192.168.200.0/24 dev tap0