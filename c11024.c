static void __sk_destruct(struct rcu_head *head)
{
	struct sock *sk = container_of(head, struct sock, sk_rcu);
	struct sk_filter *filter;

	if (sk->sk_destruct)
		sk->sk_destruct(sk);

	filter = rcu_dereference_check(sk->sk_filter,
				       refcount_read(&sk->sk_wmem_alloc) == 0);
	if (filter) {
		sk_filter_uncharge(sk, filter);
		RCU_INIT_POINTER(sk->sk_filter, NULL);
	}

	sock_disable_timestamp(sk, SK_FLAGS_TIMESTAMP);

#ifdef CONFIG_BPF_SYSCALL
	bpf_sk_storage_free(sk);
#endif

	if (atomic_read(&sk->sk_omem_alloc))
		pr_debug("%s: optmem leakage (%d bytes) detected\n",
			 __func__, atomic_read(&sk->sk_omem_alloc));

	if (sk->sk_frag.page) {
		put_page(sk->sk_frag.page);
		sk->sk_frag.page = NULL;
	}

	if (sk->sk_peer_cred)
		put_cred(sk->sk_peer_cred);
	put_pid(sk->sk_peer_pid);
	if (likely(sk->sk_net_refcnt))
		put_net(sock_net(sk));
	sk_prot_free(sk->sk_prot_creator, sk);
}