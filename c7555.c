static bool tcp_shifted_skb(struct sock *sk, struct sk_buff *prev,
			    struct sk_buff *skb,
			    struct tcp_sacktag_state *state,
			    unsigned int pcount, int shifted, int mss,
			    bool dup_sack)
{
	struct tcp_sock *tp = tcp_sk(sk);
	u32 start_seq = TCP_SKB_CB(skb)->seq;	/* start of newly-SACKed */
	u32 end_seq = start_seq + shifted;	/* end of newly-SACKed */

	BUG_ON(!pcount);

	/* Adjust counters and hints for the newly sacked sequence
	 * range but discard the return value since prev is already
	 * marked. We must tag the range first because the seq
	 * advancement below implicitly advances
	 * tcp_highest_sack_seq() when skb is highest_sack.
	 */
	tcp_sacktag_one(sk, state, TCP_SKB_CB(skb)->sacked,
			start_seq, end_seq, dup_sack, pcount,
			tcp_skb_timestamp_us(skb));
	tcp_rate_skb_delivered(sk, skb, state->rate);

	if (skb == tp->lost_skb_hint)
		tp->lost_cnt_hint += pcount;

	TCP_SKB_CB(prev)->end_seq += shifted;
	TCP_SKB_CB(skb)->seq += shifted;

	tcp_skb_pcount_add(prev, pcount);
	BUG_ON(tcp_skb_pcount(skb) < pcount);
	tcp_skb_pcount_add(skb, -pcount);

	/* When we're adding to gso_segs == 1, gso_size will be zero,
	 * in theory this shouldn't be necessary but as long as DSACK
	 * code can come after this skb later on it's better to keep
	 * setting gso_size to something.
	 */
	if (!TCP_SKB_CB(prev)->tcp_gso_size)
		TCP_SKB_CB(prev)->tcp_gso_size = mss;

	/* CHECKME: To clear or not to clear? Mimics normal skb currently */
	if (tcp_skb_pcount(skb) <= 1)
		TCP_SKB_CB(skb)->tcp_gso_size = 0;

	/* Difference in this won't matter, both ACKed by the same cumul. ACK */
	TCP_SKB_CB(prev)->sacked |= (TCP_SKB_CB(skb)->sacked & TCPCB_EVER_RETRANS);

	if (skb->len > 0) {
		BUG_ON(!tcp_skb_pcount(skb));
		NET_INC_STATS(sock_net(sk), LINUX_MIB_SACKSHIFTED);
		return false;
	}

	/* Whole SKB was eaten :-) */

	if (skb == tp->retransmit_skb_hint)
		tp->retransmit_skb_hint = prev;
	if (skb == tp->lost_skb_hint) {
		tp->lost_skb_hint = prev;
		tp->lost_cnt_hint -= tcp_skb_pcount(prev);
	}

	TCP_SKB_CB(prev)->tcp_flags |= TCP_SKB_CB(skb)->tcp_flags;
	TCP_SKB_CB(prev)->eor = TCP_SKB_CB(skb)->eor;
	if (TCP_SKB_CB(skb)->tcp_flags & TCPHDR_FIN)
		TCP_SKB_CB(prev)->end_seq++;

	if (skb == tcp_highest_sack(sk))
		tcp_advance_highest_sack(sk, skb);

	tcp_skb_collapse_tstamp(prev, skb);
	if (unlikely(TCP_SKB_CB(prev)->tx.delivered_mstamp))
		TCP_SKB_CB(prev)->tx.delivered_mstamp = 0;

	tcp_rtx_queue_unlink_and_free(skb, sk);

	NET_INC_STATS(sock_net(sk), LINUX_MIB_SACKMERGED);

	return true;
}