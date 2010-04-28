hostname R2;
interface {
	dev eth0;
	mac aa:bb:cc:00:01:11;
	gateway 127.0.0.1;
	ipaddress 199.9.9.10/24;
	port 50001;
	type hub;
}
interface {
	dev eth1;
	mac aa:bb:cc:00:01:12;
	gateway 127.0.0.1;
	ipaddress 10.10.10.10/24;
	port 50004;
	type hub;
}
