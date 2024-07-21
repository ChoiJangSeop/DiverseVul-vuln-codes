int udp_get_port(struct sock *sk, unsigned short snum,
			int (*scmp)(const struct sock *, const struct sock *))
{
	return  __udp_lib_get_port(sk, snum, udp_hash, &udp_port_rover, scmp);
}