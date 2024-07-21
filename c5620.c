static int packet_notifier(struct notifier_block *this,
			   unsigned long msg, void *ptr)
{
	struct sock *sk;
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);
	struct net *net = dev_net(dev);

	rcu_read_lock();
	sk_for_each_rcu(sk, &net->packet.sklist) {
		struct packet_sock *po = pkt_sk(sk);

		switch (msg) {
		case NETDEV_UNREGISTER:
			if (po->mclist)
				packet_dev_mclist_delete(dev, &po->mclist);
			/* fallthrough */

		case NETDEV_DOWN:
			if (dev->ifindex == po->ifindex) {
				spin_lock(&po->bind_lock);
				if (po->running) {
					__unregister_prot_hook(sk, false);
					sk->sk_err = ENETDOWN;
					if (!sock_flag(sk, SOCK_DEAD))
						sk->sk_error_report(sk);
				}
				if (msg == NETDEV_UNREGISTER) {
					packet_cached_dev_reset(po);
					fanout_release(sk);
					po->ifindex = -1;
					if (po->prot_hook.dev)
						dev_put(po->prot_hook.dev);
					po->prot_hook.dev = NULL;
				}
				spin_unlock(&po->bind_lock);
			}
			break;
		case NETDEV_UP:
			if (dev->ifindex == po->ifindex) {
				spin_lock(&po->bind_lock);
				if (po->num)
					register_prot_hook(sk);
				spin_unlock(&po->bind_lock);
			}
			break;
		}
	}
	rcu_read_unlock();
	return NOTIFY_DONE;
}