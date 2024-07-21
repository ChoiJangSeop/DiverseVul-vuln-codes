static int dummy_ipv6_recv_error(struct sock *sk, struct msghdr *msg, int len)
{
	return -EAFNOSUPPORT;
}