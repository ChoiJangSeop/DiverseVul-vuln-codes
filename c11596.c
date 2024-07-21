static void rose_heartbeat_expiry(struct timer_list *t)
{
	struct sock *sk = from_timer(sk, t, sk_timer);
	struct rose_sock *rose = rose_sk(sk);

	bh_lock_sock(sk);
	switch (rose->state) {
	case ROSE_STATE_0:
		/* Magic here: If we listen() and a new link dies before it
		   is accepted() it isn't 'dead' so doesn't get removed. */
		if (sock_flag(sk, SOCK_DESTROY) ||
		    (sk->sk_state == TCP_LISTEN && sock_flag(sk, SOCK_DEAD))) {
			bh_unlock_sock(sk);
			rose_destroy_socket(sk);
			return;
		}
		break;

	case ROSE_STATE_3:
		/*
		 * Check for the state of the receive buffer.
		 */
		if (atomic_read(&sk->sk_rmem_alloc) < (sk->sk_rcvbuf / 2) &&
		    (rose->condition & ROSE_COND_OWN_RX_BUSY)) {
			rose->condition &= ~ROSE_COND_OWN_RX_BUSY;
			rose->condition &= ~ROSE_COND_ACK_PENDING;
			rose->vl         = rose->vr;
			rose_write_internal(sk, ROSE_RR);
			rose_stop_timer(sk);	/* HB */
			break;
		}
		break;
	}

	rose_start_heartbeat(sk);
	bh_unlock_sock(sk);
}