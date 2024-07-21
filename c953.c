void cipso_v4_sock_delattr(struct sock *sk)
{
	int hdr_delta;
	struct ip_options *opt;
	struct inet_sock *sk_inet;

	sk_inet = inet_sk(sk);
	opt = sk_inet->opt;
	if (opt == NULL || opt->cipso == 0)
		return;

	hdr_delta = cipso_v4_delopt(&sk_inet->opt);
	if (sk_inet->is_icsk && hdr_delta > 0) {
		struct inet_connection_sock *sk_conn = inet_csk(sk);
		sk_conn->icsk_ext_hdr_len -= hdr_delta;
		sk_conn->icsk_sync_mss(sk, sk_conn->icsk_pmtu_cookie);
	}
}