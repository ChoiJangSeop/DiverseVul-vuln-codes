static int eafnosupport_ipv6_dst_lookup(struct net *net, struct sock *u1,
					struct dst_entry **u2,
					struct flowi6 *u3)
{
	return -EAFNOSUPPORT;
}