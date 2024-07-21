int __udp_lib_get_port(struct sock *sk, unsigned short snum,
		       struct hlist_head udptable[], int *port_rover,
		       int (*saddr_comp)(const struct sock *sk1,
					 const struct sock *sk2 )    )
{
	struct hlist_node *node;
	struct hlist_head *head;
	struct sock *sk2;
	int    error = 1;

	write_lock_bh(&udp_hash_lock);
	if (snum == 0) {
		int best_size_so_far, best, result, i;

		if (*port_rover > sysctl_local_port_range[1] ||
		    *port_rover < sysctl_local_port_range[0])
			*port_rover = sysctl_local_port_range[0];
		best_size_so_far = 32767;
		best = result = *port_rover;
		for (i = 0; i < UDP_HTABLE_SIZE; i++, result++) {
			int size;

			head = &udptable[result & (UDP_HTABLE_SIZE - 1)];
			if (hlist_empty(head)) {
				if (result > sysctl_local_port_range[1])
					result = sysctl_local_port_range[0] +
						((result - sysctl_local_port_range[0]) &
						 (UDP_HTABLE_SIZE - 1));
				goto gotit;
			}
			size = 0;
			sk_for_each(sk2, node, head) {
				if (++size >= best_size_so_far)
					goto next;
			}
			best_size_so_far = size;
			best = result;
		next:
			;
		}
		result = best;
		for (i = 0; i < (1 << 16) / UDP_HTABLE_SIZE;
		     i++, result += UDP_HTABLE_SIZE) {
			if (result > sysctl_local_port_range[1])
				result = sysctl_local_port_range[0]
					+ ((result - sysctl_local_port_range[0]) &
					   (UDP_HTABLE_SIZE - 1));
			if (! __udp_lib_lport_inuse(result, udptable))
				break;
		}
		if (i >= (1 << 16) / UDP_HTABLE_SIZE)
			goto fail;
gotit:
		*port_rover = snum = result;
	} else {
		head = &udptable[snum & (UDP_HTABLE_SIZE - 1)];

		sk_for_each(sk2, node, head)
			if (sk2->sk_hash == snum                             &&
			    sk2 != sk                                        &&
			    (!sk2->sk_reuse        || !sk->sk_reuse)         &&
			    (!sk2->sk_bound_dev_if || !sk->sk_bound_dev_if
			     || sk2->sk_bound_dev_if == sk->sk_bound_dev_if) &&
			    (*saddr_comp)(sk, sk2)                             )
				goto fail;
	}
	inet_sk(sk)->num = snum;
	sk->sk_hash = snum;
	if (sk_unhashed(sk)) {
		head = &udptable[snum & (UDP_HTABLE_SIZE - 1)];
		sk_add_node(sk, head);
		sock_prot_inc_use(sk->sk_prot);
	}
	error = 0;
fail:
	write_unlock_bh(&udp_hash_lock);
	return error;
}