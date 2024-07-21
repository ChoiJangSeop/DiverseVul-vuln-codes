static void rose_idletimer_expiry(struct timer_list *t)
{
	struct rose_sock *rose = from_timer(rose, t, idletimer);
	struct sock *sk = &rose->sock;

	bh_lock_sock(sk);
	rose_clear_queues(sk);

	rose_write_internal(sk, ROSE_CLEAR_REQUEST);
	rose_sk(sk)->state = ROSE_STATE_2;

	rose_start_t3timer(sk);

	sk->sk_state     = TCP_CLOSE;
	sk->sk_err       = 0;
	sk->sk_shutdown |= SEND_SHUTDOWN;

	if (!sock_flag(sk, SOCK_DEAD)) {
		sk->sk_state_change(sk);
		sock_set_flag(sk, SOCK_DEAD);
	}
	bh_unlock_sock(sk);
}