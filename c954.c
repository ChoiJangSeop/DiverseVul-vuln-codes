int dccp_v4_connect(struct sock *sk, struct sockaddr *uaddr, int addr_len)
{
	const struct sockaddr_in *usin = (struct sockaddr_in *)uaddr;
	struct inet_sock *inet = inet_sk(sk);
	struct dccp_sock *dp = dccp_sk(sk);
	__be16 orig_sport, orig_dport;
	__be32 daddr, nexthop;
	struct flowi4 fl4;
	struct rtable *rt;
	int err;

	dp->dccps_role = DCCP_ROLE_CLIENT;

	if (addr_len < sizeof(struct sockaddr_in))
		return -EINVAL;

	if (usin->sin_family != AF_INET)
		return -EAFNOSUPPORT;

	nexthop = daddr = usin->sin_addr.s_addr;
	if (inet->opt != NULL && inet->opt->srr) {
		if (daddr == 0)
			return -EINVAL;
		nexthop = inet->opt->faddr;
	}

	orig_sport = inet->inet_sport;
	orig_dport = usin->sin_port;
	rt = ip_route_connect(&fl4, nexthop, inet->inet_saddr,
			      RT_CONN_FLAGS(sk), sk->sk_bound_dev_if,
			      IPPROTO_DCCP,
			      orig_sport, orig_dport, sk, true);
	if (IS_ERR(rt))
		return PTR_ERR(rt);

	if (rt->rt_flags & (RTCF_MULTICAST | RTCF_BROADCAST)) {
		ip_rt_put(rt);
		return -ENETUNREACH;
	}

	if (inet->opt == NULL || !inet->opt->srr)
		daddr = rt->rt_dst;

	if (inet->inet_saddr == 0)
		inet->inet_saddr = rt->rt_src;
	inet->inet_rcv_saddr = inet->inet_saddr;

	inet->inet_dport = usin->sin_port;
	inet->inet_daddr = daddr;

	inet_csk(sk)->icsk_ext_hdr_len = 0;
	if (inet->opt != NULL)
		inet_csk(sk)->icsk_ext_hdr_len = inet->opt->optlen;
	/*
	 * Socket identity is still unknown (sport may be zero).
	 * However we set state to DCCP_REQUESTING and not releasing socket
	 * lock select source port, enter ourselves into the hash tables and
	 * complete initialization after this.
	 */
	dccp_set_state(sk, DCCP_REQUESTING);
	err = inet_hash_connect(&dccp_death_row, sk);
	if (err != 0)
		goto failure;

	rt = ip_route_newports(&fl4, rt, orig_sport, orig_dport,
			       inet->inet_sport, inet->inet_dport, sk);
	if (IS_ERR(rt)) {
		rt = NULL;
		goto failure;
	}
	/* OK, now commit destination to socket.  */
	sk_setup_caps(sk, &rt->dst);

	dp->dccps_iss = secure_dccp_sequence_number(inet->inet_saddr,
						    inet->inet_daddr,
						    inet->inet_sport,
						    inet->inet_dport);
	inet->inet_id = dp->dccps_iss ^ jiffies;

	err = dccp_connect(sk);
	rt = NULL;
	if (err != 0)
		goto failure;
out:
	return err;
failure:
	/*
	 * This unhashes the socket and releases the local port, if necessary.
	 */
	dccp_set_state(sk, DCCP_CLOSED);
	ip_rt_put(rt);
	sk->sk_route_caps = 0;
	inet->inet_dport = 0;
	goto out;
}