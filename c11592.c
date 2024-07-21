void rose_stop_idletimer(struct sock *sk)
{
	del_timer(&rose_sk(sk)->idletimer);
}