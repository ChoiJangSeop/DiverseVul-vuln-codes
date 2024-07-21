struct sock *cookie_v4_check(struct sock *sk, struct sk_buff *skb,
			     struct ip_options *opt)
{
	struct tcp_options_received tcp_opt;
	u8 *hash_location;
	struct inet_request_sock *ireq;
	struct tcp_request_sock *treq;
	struct tcp_sock *tp = tcp_sk(sk);
	const struct tcphdr *th = tcp_hdr(skb);
	__u32 cookie = ntohl(th->ack_seq) - 1;
	struct sock *ret = sk;
	struct request_sock *req;
	int mss;
	struct rtable *rt;
	__u8 rcv_wscale;
	bool ecn_ok;

	if (!sysctl_tcp_syncookies || !th->ack || th->rst)
		goto out;

	if (tcp_synq_no_recent_overflow(sk) ||
	    (mss = cookie_check(skb, cookie)) == 0) {
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_SYNCOOKIESFAILED);
		goto out;
	}

	NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_SYNCOOKIESRECV);

	/* check for timestamp cookie support */
	memset(&tcp_opt, 0, sizeof(tcp_opt));
	tcp_parse_options(skb, &tcp_opt, &hash_location, 0);

	if (!cookie_check_timestamp(&tcp_opt, &ecn_ok))
		goto out;

	ret = NULL;
	req = inet_reqsk_alloc(&tcp_request_sock_ops); /* for safety */
	if (!req)
		goto out;

	ireq = inet_rsk(req);
	treq = tcp_rsk(req);
	treq->rcv_isn		= ntohl(th->seq) - 1;
	treq->snt_isn		= cookie;
	req->mss		= mss;
	ireq->loc_port		= th->dest;
	ireq->rmt_port		= th->source;
	ireq->loc_addr		= ip_hdr(skb)->daddr;
	ireq->rmt_addr		= ip_hdr(skb)->saddr;
	ireq->ecn_ok		= ecn_ok;
	ireq->snd_wscale	= tcp_opt.snd_wscale;
	ireq->sack_ok		= tcp_opt.sack_ok;
	ireq->wscale_ok		= tcp_opt.wscale_ok;
	ireq->tstamp_ok		= tcp_opt.saw_tstamp;
	req->ts_recent		= tcp_opt.saw_tstamp ? tcp_opt.rcv_tsval : 0;

	/* We throwed the options of the initial SYN away, so we hope
	 * the ACK carries the same options again (see RFC1122 4.2.3.8)
	 */
	if (opt && opt->optlen) {
		int opt_size = sizeof(struct ip_options) + opt->optlen;

		ireq->opt = kmalloc(opt_size, GFP_ATOMIC);
		if (ireq->opt != NULL && ip_options_echo(ireq->opt, skb)) {
			kfree(ireq->opt);
			ireq->opt = NULL;
		}
	}

	if (security_inet_conn_request(sk, skb, req)) {
		reqsk_free(req);
		goto out;
	}

	req->expires	= 0UL;
	req->retrans	= 0;

	/*
	 * We need to lookup the route here to get at the correct
	 * window size. We should better make sure that the window size
	 * hasn't changed since we received the original syn, but I see
	 * no easy way to do this.
	 */
	{
		struct flowi4 fl4;

		flowi4_init_output(&fl4, 0, sk->sk_mark, RT_CONN_FLAGS(sk),
				   RT_SCOPE_UNIVERSE, IPPROTO_TCP,
				   inet_sk_flowi_flags(sk),
				   (opt && opt->srr) ? opt->faddr : ireq->rmt_addr,
				   ireq->loc_addr, th->source, th->dest);
		security_req_classify_flow(req, flowi4_to_flowi(&fl4));
		rt = ip_route_output_key(sock_net(sk), &fl4);
		if (IS_ERR(rt)) {
			reqsk_free(req);
			goto out;
		}
	}

	/* Try to redo what tcp_v4_send_synack did. */
	req->window_clamp = tp->window_clamp ? :dst_metric(&rt->dst, RTAX_WINDOW);

	tcp_select_initial_window(tcp_full_space(sk), req->mss,
				  &req->rcv_wnd, &req->window_clamp,
				  ireq->wscale_ok, &rcv_wscale,
				  dst_metric(&rt->dst, RTAX_INITRWND));

	ireq->rcv_wscale  = rcv_wscale;

	ret = get_cookie_sock(sk, skb, req, &rt->dst);
out:	return ret;
}