static void init_peercred(struct sock *sk)
{
	put_pid(sk->sk_peer_pid);
	if (sk->sk_peer_cred)
		put_cred(sk->sk_peer_cred);
	sk->sk_peer_pid  = get_pid(task_tgid(current));
	sk->sk_peer_cred = get_current_cred();
}