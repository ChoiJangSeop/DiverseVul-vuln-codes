void rose_stop_heartbeat(struct sock *sk)
{
	del_timer(&sk->sk_timer);
}