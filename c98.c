int udplite_get_port(struct sock *sk, unsigned short p,
		     int (*c)(const struct sock *, const struct sock *))
{
	return  __udp_lib_get_port(sk, p, udplite_hash, &udplite_port_rover, c);
}