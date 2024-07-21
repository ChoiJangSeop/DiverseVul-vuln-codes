static bool tcp_collapse_retrans(struct sock *sk, struct sk_buff *skb)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct sk_buff *next_skb = skb_rb_next(skb);
	int next_skb_size;

	next_skb_size = next_skb->len;

	BUG_ON(tcp_skb_pcount(skb) != 1 || tcp_skb_pcount(next_skb) != 1);

	if (next_skb_size) {
		if (next_skb_size <= skb_availroom(skb))
			skb_copy_bits(next_skb, 0, skb_put(skb, next_skb_size),
				      next_skb_size);
		else if (!skb_shift(skb, next_skb, next_skb_size))
			return false;
	}
	tcp_highest_sack_replace(sk, next_skb, skb);

	/* Update sequence range on original skb. */
	TCP_SKB_CB(skb)->end_seq = TCP_SKB_CB(next_skb)->end_seq;

	/* Merge over control information. This moves PSH/FIN etc. over */
	TCP_SKB_CB(skb)->tcp_flags |= TCP_SKB_CB(next_skb)->tcp_flags;

	/* All done, get rid of second SKB and account for it so
	 * packet counting does not break.
	 */
	TCP_SKB_CB(skb)->sacked |= TCP_SKB_CB(next_skb)->sacked & TCPCB_EVER_RETRANS;
	TCP_SKB_CB(skb)->eor = TCP_SKB_CB(next_skb)->eor;

	/* changed transmit queue under us so clear hints */
	tcp_clear_retrans_hints_partial(tp);
	if (next_skb == tp->retransmit_skb_hint)
		tp->retransmit_skb_hint = skb;

	tcp_adjust_pcount(sk, next_skb, tcp_skb_pcount(next_skb));

	tcp_skb_collapse_tstamp(skb, next_skb);

	tcp_rtx_queue_unlink_and_free(next_skb, sk);
	return true;
}