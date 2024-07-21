void rose_stop_timer(struct sock *sk)
{
	del_timer(&rose_sk(sk)->timer);
}