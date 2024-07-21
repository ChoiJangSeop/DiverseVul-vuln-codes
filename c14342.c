static void l2cap_sock_destruct(struct sock *sk)
{
	BT_DBG("sk %p", sk);

	if (l2cap_pi(sk)->chan)
		l2cap_chan_put(l2cap_pi(sk)->chan);

	if (l2cap_pi(sk)->rx_busy_skb) {
		kfree_skb(l2cap_pi(sk)->rx_busy_skb);
		l2cap_pi(sk)->rx_busy_skb = NULL;
	}

	skb_queue_purge(&sk->sk_receive_queue);
	skb_queue_purge(&sk->sk_write_queue);
}