hostname R3;
interface {
	dev eth0;
	mac aa:bb:cc:00:01:21;
	gateway 127.0.0.1;
	ipaddress 8.8.8.9/24;
	port 50002;
	type hub;
	link hub2;
}
interface {
	dev eth1;
	mac aa:bb:cc:00:01:22;
	gateway 127.0.0.1;
	ipaddress 168.16.21.9/24;
	port 50003;
	type hub;
	link hub3;
}
