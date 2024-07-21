static void flush_stack(struct sock **stack, unsigned int count,
			struct sk_buff *skb, unsigned int final)
{
	unsigned int i;
	struct sock *sk;
	struct sk_buff *skb1;

	for (i = 0; i < count; i++) {
		skb1 = (i == final) ? skb : skb_clone(skb, GFP_ATOMIC);

		sk = stack[i];
		if (skb1) {
			bh_lock_sock(sk);
			if (!sock_owned_by_user(sk))
				udpv6_queue_rcv_skb(sk, skb1);
			else if (sk_add_backlog(sk, skb1)) {
				kfree_skb(skb1);
				bh_unlock_sock(sk);
				goto drop;
			}
			bh_unlock_sock(sk);
			continue;
		}
drop:
		atomic_inc(&sk->sk_drops);
		UDP6_INC_STATS_BH(sock_net(sk),
				UDP_MIB_RCVBUFERRORS, IS_UDPLITE(sk));
		UDP6_INC_STATS_BH(sock_net(sk),
				UDP_MIB_INERRORS, IS_UDPLITE(sk));
	}
}