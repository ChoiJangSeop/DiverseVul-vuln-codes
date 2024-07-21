static void scan_inflight(struct sock *x, void (*func)(struct unix_sock *),
			  struct sk_buff_head *hitlist)
{
	struct sk_buff *skb;
	struct sk_buff *next;

	spin_lock(&x->sk_receive_queue.lock);
	receive_queue_for_each_skb(x, next, skb) {
		/*
		 *	Do we have file descriptors ?
		 */
		if (UNIXCB(skb).fp) {
			bool hit = false;
			/*
			 *	Process the descriptors of this socket
			 */
			int nfd = UNIXCB(skb).fp->count;
			struct file **fp = UNIXCB(skb).fp->fp;
			while (nfd--) {
				/*
				 *	Get the socket the fd matches
				 *	if it indeed does so
				 */
				struct sock *sk = unix_get_socket(*fp++);
				if (sk) {
					hit = true;
					func(unix_sk(sk));
				}
			}
			if (hit && hitlist != NULL) {
				__skb_unlink(skb, &x->sk_receive_queue);
				__skb_queue_tail(hitlist, skb);
			}
		}
	}
	spin_unlock(&x->sk_receive_queue.lock);
}