static int dccp_v6_send_response(const struct sock *sk, struct request_sock *req)
{
	struct inet_request_sock *ireq = inet_rsk(req);
	struct ipv6_pinfo *np = inet6_sk(sk);
	struct sk_buff *skb;
	struct in6_addr *final_p, final;
	struct flowi6 fl6;
	int err = -1;
	struct dst_entry *dst;

	memset(&fl6, 0, sizeof(fl6));
	fl6.flowi6_proto = IPPROTO_DCCP;
	fl6.daddr = ireq->ir_v6_rmt_addr;
	fl6.saddr = ireq->ir_v6_loc_addr;
	fl6.flowlabel = 0;
	fl6.flowi6_oif = ireq->ir_iif;
	fl6.fl6_dport = ireq->ir_rmt_port;
	fl6.fl6_sport = htons(ireq->ir_num);
	security_req_classify_flow(req, flowi6_to_flowi(&fl6));


	final_p = fl6_update_dst(&fl6, np->opt, &final);

	dst = ip6_dst_lookup_flow(sk, &fl6, final_p);
	if (IS_ERR(dst)) {
		err = PTR_ERR(dst);
		dst = NULL;
		goto done;
	}

	skb = dccp_make_response(sk, dst, req);
	if (skb != NULL) {
		struct dccp_hdr *dh = dccp_hdr(skb);

		dh->dccph_checksum = dccp_v6_csum_finish(skb,
							 &ireq->ir_v6_loc_addr,
							 &ireq->ir_v6_rmt_addr);
		fl6.daddr = ireq->ir_v6_rmt_addr;
		err = ip6_xmit(sk, skb, &fl6, np->opt, np->tclass);
		err = net_xmit_eval(err);
	}

done:
	dst_release(dst);
	return err;
}