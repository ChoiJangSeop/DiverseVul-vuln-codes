static bool tcp_dma_try_early_copy(struct sock *sk, struct sk_buff *skb,
				  int hlen)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int chunk = skb->len - hlen;
	int dma_cookie;
	bool copied_early = false;

	if (tp->ucopy.wakeup)
		return false;

	if (!tp->ucopy.dma_chan && tp->ucopy.pinned_list)
		tp->ucopy.dma_chan = net_dma_find_channel();

	if (tp->ucopy.dma_chan && skb_csum_unnecessary(skb)) {

		dma_cookie = dma_skb_copy_datagram_iovec(tp->ucopy.dma_chan,
							 skb, hlen,
							 tp->ucopy.iov, chunk,
							 tp->ucopy.pinned_list);

		if (dma_cookie < 0)
			goto out;

		tp->ucopy.dma_cookie = dma_cookie;
		copied_early = true;

		tp->ucopy.len -= chunk;
		tp->copied_seq += chunk;
		tcp_rcv_space_adjust(sk);

		if ((tp->ucopy.len == 0) ||
		    (tcp_flag_word(tcp_hdr(skb)) & TCP_FLAG_PSH) ||
		    (atomic_read(&sk->sk_rmem_alloc) > (sk->sk_rcvbuf >> 1))) {
			tp->ucopy.wakeup = 1;
			sk->sk_data_ready(sk, 0);
		}
	} else if (chunk > 0) {
		tp->ucopy.wakeup = 1;
		sk->sk_data_ready(sk, 0);
	}
out:
	return copied_early;
}