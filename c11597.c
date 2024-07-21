void rose_start_idletimer(struct sock *sk)
{
	struct rose_sock *rose = rose_sk(sk);

	del_timer(&rose->idletimer);

	if (rose->idle > 0) {
		rose->idletimer.function = rose_idletimer_expiry;
		rose->idletimer.expires  = jiffies + rose->idle;

		add_timer(&rose->idletimer);
	}
}