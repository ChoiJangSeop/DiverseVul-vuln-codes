void rose_start_t3timer(struct sock *sk)
{
	struct rose_sock *rose = rose_sk(sk);

	del_timer(&rose->timer);

	rose->timer.function = rose_timer_expiry;
	rose->timer.expires  = jiffies + rose->t3;

	add_timer(&rose->timer);
}