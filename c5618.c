static int packet_release(struct socket *sock)
{
	struct sock *sk = sock->sk;
	struct packet_sock *po;
	struct net *net;
	union tpacket_req_u req_u;

	if (!sk)
		return 0;

	net = sock_net(sk);
	po = pkt_sk(sk);

	mutex_lock(&net->packet.sklist_lock);
	sk_del_node_init_rcu(sk);
	mutex_unlock(&net->packet.sklist_lock);

	preempt_disable();
	sock_prot_inuse_add(net, sk->sk_prot, -1);
	preempt_enable();

	spin_lock(&po->bind_lock);
	unregister_prot_hook(sk, false);
	packet_cached_dev_reset(po);

	if (po->prot_hook.dev) {
		dev_put(po->prot_hook.dev);
		po->prot_hook.dev = NULL;
	}
	spin_unlock(&po->bind_lock);

	packet_flush_mclist(sk);

	if (po->rx_ring.pg_vec) {
		memset(&req_u, 0, sizeof(req_u));
		packet_set_ring(sk, &req_u, 1, 0);
	}

	if (po->tx_ring.pg_vec) {
		memset(&req_u, 0, sizeof(req_u));
		packet_set_ring(sk, &req_u, 1, 1);
	}

	fanout_release(sk);

	synchronize_net();
	/*
	 *	Now the socket is dead. No more input will appear.
	 */
	sock_orphan(sk);
	sock->sk = NULL;

	/* Purge queues */

	skb_queue_purge(&sk->sk_receive_queue);
	packet_free_pending(po);
	sk_refcnt_debug_release(sk);

	sock_put(sk);
	return 0;
}