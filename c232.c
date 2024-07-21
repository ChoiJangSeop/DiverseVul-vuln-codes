void unix_gc(void)
{
	static DEFINE_MUTEX(unix_gc_sem);
	int i;
	struct sock *s;
	struct sk_buff_head hitlist;
	struct sk_buff *skb;

	/*
	 *	Avoid a recursive GC.
	 */

	if (!mutex_trylock(&unix_gc_sem))
		return;

	spin_lock(&unix_table_lock);

	forall_unix_sockets(i, s)
	{
		unix_sk(s)->gc_tree = GC_ORPHAN;
	}
	/*
	 *	Everything is now marked
	 */

	/* Invariant to be maintained:
		- everything unmarked is either:
		-- (a) on the stack, or
		-- (b) has all of its children unmarked
		- everything on the stack is always unmarked
		- nothing is ever pushed onto the stack twice, because:
		-- nothing previously unmarked is ever pushed on the stack
	 */

	/*
	 *	Push root set
	 */

	forall_unix_sockets(i, s)
	{
		int open_count = 0;

		/*
		 *	If all instances of the descriptor are not
		 *	in flight we are in use.
		 *
		 *	Special case: when socket s is embrion, it may be
		 *	hashed but still not in queue of listening socket.
		 *	In this case (see unix_create1()) we set artificial
		 *	negative inflight counter to close race window.
		 *	It is trick of course and dirty one.
		 */
		if (s->sk_socket && s->sk_socket->file)
			open_count = file_count(s->sk_socket->file);
		if (open_count > atomic_read(&unix_sk(s)->inflight))
			maybe_unmark_and_push(s);
	}

	/*
	 *	Mark phase
	 */

	while (!empty_stack())
	{
		struct sock *x = pop_stack();
		struct sock *sk;

		spin_lock(&x->sk_receive_queue.lock);
		skb = skb_peek(&x->sk_receive_queue);

		/*
		 *	Loop through all but first born
		 */

		while (skb && skb != (struct sk_buff *)&x->sk_receive_queue) {
			/*
			 *	Do we have file descriptors ?
			 */
			if(UNIXCB(skb).fp)
			{
				/*
				 *	Process the descriptors of this socket
				 */
				int nfd=UNIXCB(skb).fp->count;
				struct file **fp = UNIXCB(skb).fp->fp;
				while(nfd--)
				{
					/*
					 *	Get the socket the fd matches if
					 *	it indeed does so
					 */
					if((sk=unix_get_socket(*fp++))!=NULL)
					{
						maybe_unmark_and_push(sk);
					}
				}
			}
			/* We have to scan not-yet-accepted ones too */
			if (x->sk_state == TCP_LISTEN)
				maybe_unmark_and_push(skb->sk);
			skb=skb->next;
		}
		spin_unlock(&x->sk_receive_queue.lock);
		sock_put(x);
	}

	skb_queue_head_init(&hitlist);

	forall_unix_sockets(i, s)
	{
		struct unix_sock *u = unix_sk(s);

		if (u->gc_tree == GC_ORPHAN) {
			struct sk_buff *nextsk;

			spin_lock(&s->sk_receive_queue.lock);
			skb = skb_peek(&s->sk_receive_queue);
			while (skb &&
			       skb != (struct sk_buff *)&s->sk_receive_queue) {
				nextsk = skb->next;
				/*
				 *	Do we have file descriptors ?
				 */
				if (UNIXCB(skb).fp) {
					__skb_unlink(skb,
						     &s->sk_receive_queue);
					__skb_queue_tail(&hitlist, skb);
				}
				skb = nextsk;
			}
			spin_unlock(&s->sk_receive_queue.lock);
		}
		u->gc_tree = GC_ORPHAN;
	}
	spin_unlock(&unix_table_lock);

	/*
	 *	Here we are. Hitlist is filled. Die.
	 */

	__skb_queue_purge(&hitlist);
	mutex_unlock(&unix_gc_sem);
}