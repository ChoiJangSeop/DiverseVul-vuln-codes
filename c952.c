struct sock *dccp_v4_request_recv_sock(struct sock *sk, struct sk_buff *skb,
				       struct request_sock *req,
				       struct dst_entry *dst)
{
	struct inet_request_sock *ireq;
	struct inet_sock *newinet;
	struct sock *newsk;

	if (sk_acceptq_is_full(sk))
		goto exit_overflow;

	if (dst == NULL && (dst = inet_csk_route_req(sk, req)) == NULL)
		goto exit;

	newsk = dccp_create_openreq_child(sk, req, skb);
	if (newsk == NULL)
		goto exit_nonewsk;

	sk_setup_caps(newsk, dst);

	newinet		   = inet_sk(newsk);
	ireq		   = inet_rsk(req);
	newinet->inet_daddr	= ireq->rmt_addr;
	newinet->inet_rcv_saddr = ireq->loc_addr;
	newinet->inet_saddr	= ireq->loc_addr;
	newinet->opt	   = ireq->opt;
	ireq->opt	   = NULL;
	newinet->mc_index  = inet_iif(skb);
	newinet->mc_ttl	   = ip_hdr(skb)->ttl;
	newinet->inet_id   = jiffies;

	dccp_sync_mss(newsk, dst_mtu(dst));

	if (__inet_inherit_port(sk, newsk) < 0) {
		sock_put(newsk);
		goto exit;
	}
	__inet_hash_nolisten(newsk, NULL);

	return newsk;

exit_overflow:
	NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_LISTENOVERFLOWS);
exit_nonewsk:
	dst_release(dst);
exit:
	NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_LISTENDROPS);
	return NULL;
}