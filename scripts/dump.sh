#!/bin/bash

what=$1

if [ $what == "arpreqdns" ]
then
    traffic_type="arp"
    tcpdump -ne -i tap0 $traffic_type
fi

if [ $what == "dnsreq" ]
then
    tcpdump -ne -i tap0 "udp and dst port 53"
fi

if [ $what == "dnsresp" ]
then
    tcpdump -ne -i tap0 "udp and src port 53"
fi

if [ $what == "ttl" ]
then
    where=$2
    if [ $where == "b" ]
    then
	tcpdump -nveX -i tap2 "src 192.168.100.123 and dst 192.168.200.123"
    else
	tcpdump -nveX -i tap3 "src 192.168.100.123 and dst 192.168.200.123"
    fi
fi

if [ $what == "syn" ]
then
    tcpdump -ne -i tap0 "tcp[13] & 2 != 0"
fi

if [ $what == "page" ]
then
    tcpdump -nveX -i tap0 "tcp[13] & 2 == 0"
fi
