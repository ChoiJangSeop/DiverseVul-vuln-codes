static void tcp_service_net_dma(struct sock *sk, bool wait)
{
	dma_cookie_t done, used;
	dma_cookie_t last_issued;
	struct tcp_sock *tp = tcp_sk(sk);

	if (!tp->ucopy.dma_chan)
		return;

	last_issued = tp->ucopy.dma_cookie;
	dma_async_issue_pending(tp->ucopy.dma_chan);

	do {
		if (dma_async_is_tx_complete(tp->ucopy.dma_chan,
					      last_issued, &done,
					      &used) == DMA_COMPLETE) {
			/* Safe to free early-copied skbs now */
			__skb_queue_purge(&sk->sk_async_wait_queue);
			break;
		} else {
			struct sk_buff *skb;
			while ((skb = skb_peek(&sk->sk_async_wait_queue)) &&
			       (dma_async_is_complete(skb->dma_cookie, done,
						      used) == DMA_COMPLETE)) {
				__skb_dequeue(&sk->sk_async_wait_queue);
				kfree_skb(skb);
			}
		}
	} while (wait);
}