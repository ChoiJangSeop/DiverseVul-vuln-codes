int cipso_v4_sock_getattr(struct sock *sk, struct netlbl_lsm_secattr *secattr)
{
	struct ip_options *opt;

	opt = inet_sk(sk)->opt;
	if (opt == NULL || opt->cipso == 0)
		return -ENOMSG;

	return cipso_v4_getattr(opt->__data + opt->cipso - sizeof(struct iphdr),
				secattr);
}