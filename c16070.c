static int inet6_create(struct net *net, struct socket *sock, int protocol)
{
	struct inet_sock *inet;
	struct ipv6_pinfo *np;
	struct sock *sk;
	struct list_head *p;
	struct inet_protosw *answer;
	struct proto *answer_prot;
	unsigned char answer_flags;
	char answer_no_check;
	int try_loading_module = 0;
	int err;

	if (sock->type != SOCK_RAW &&
	    sock->type != SOCK_DGRAM &&
	    !inet_ehash_secret)
		build_ehash_secret();

	/* Look for the requested type/protocol pair. */
	answer = NULL;
lookup_protocol:
	err = -ESOCKTNOSUPPORT;
	rcu_read_lock();
	list_for_each_rcu(p, &inetsw6[sock->type]) {
		answer = list_entry(p, struct inet_protosw, list);

		/* Check the non-wild match. */
		if (protocol == answer->protocol) {
			if (protocol != IPPROTO_IP)
				break;
		} else {
			/* Check for the two wild cases. */
			if (IPPROTO_IP == protocol) {
				protocol = answer->protocol;
				break;
			}
			if (IPPROTO_IP == answer->protocol)
				break;
		}
		err = -EPROTONOSUPPORT;
		answer = NULL;
	}

	if (!answer) {
		if (try_loading_module < 2) {
			rcu_read_unlock();
			/*
			 * Be more specific, e.g. net-pf-10-proto-132-type-1
			 * (net-pf-PF_INET6-proto-IPPROTO_SCTP-type-SOCK_STREAM)
			 */
			if (++try_loading_module == 1)
				request_module("net-pf-%d-proto-%d-type-%d",
						PF_INET6, protocol, sock->type);
			/*
			 * Fall back to generic, e.g. net-pf-10-proto-132
			 * (net-pf-PF_INET6-proto-IPPROTO_SCTP)
			 */
			else
				request_module("net-pf-%d-proto-%d",
						PF_INET6, protocol);
			goto lookup_protocol;
		} else
			goto out_rcu_unlock;
	}

	err = -EPERM;
	if (answer->capability > 0 && !capable(answer->capability))
		goto out_rcu_unlock;

	sock->ops = answer->ops;
	answer_prot = answer->prot;
	answer_no_check = answer->no_check;
	answer_flags = answer->flags;
	rcu_read_unlock();

	BUG_TRAP(answer_prot->slab != NULL);

	err = -ENOBUFS;
	sk = sk_alloc(net, PF_INET6, GFP_KERNEL, answer_prot);
	if (sk == NULL)
		goto out;

	sock_init_data(sock, sk);

	err = 0;
	sk->sk_no_check = answer_no_check;
	if (INET_PROTOSW_REUSE & answer_flags)
		sk->sk_reuse = 1;

	inet = inet_sk(sk);
	inet->is_icsk = (INET_PROTOSW_ICSK & answer_flags) != 0;

	if (SOCK_RAW == sock->type) {
		inet->num = protocol;
		if (IPPROTO_RAW == protocol)
			inet->hdrincl = 1;
	}

	sk->sk_destruct		= inet_sock_destruct;
	sk->sk_family		= PF_INET6;
	sk->sk_protocol		= protocol;

	sk->sk_backlog_rcv	= answer->prot->backlog_rcv;

	inet_sk(sk)->pinet6 = np = inet6_sk_generic(sk);
	np->hop_limit	= -1;
	np->mcast_hops	= -1;
	np->mc_loop	= 1;
	np->pmtudisc	= IPV6_PMTUDISC_WANT;
	np->ipv6only	= init_net.ipv6.sysctl.bindv6only;

	/* Init the ipv4 part of the socket since we can have sockets
	 * using v6 API for ipv4.
	 */
	inet->uc_ttl	= -1;

	inet->mc_loop	= 1;
	inet->mc_ttl	= 1;
	inet->mc_index	= 0;
	inet->mc_list	= NULL;

	if (ipv4_config.no_pmtu_disc)
		inet->pmtudisc = IP_PMTUDISC_DONT;
	else
		inet->pmtudisc = IP_PMTUDISC_WANT;
	/*
	 * Increment only the relevant sk_prot->socks debug field, this changes
	 * the previous behaviour of incrementing both the equivalent to
	 * answer->prot->socks (inet6_sock_nr) and inet_sock_nr.
	 *
	 * This allows better debug granularity as we'll know exactly how many
	 * UDPv6, TCPv6, etc socks were allocated, not the sum of all IPv6
	 * transport protocol socks. -acme
	 */
	sk_refcnt_debug_inc(sk);

	if (inet->num) {
		/* It assumes that any protocol which allows
		 * the user to assign a number at socket
		 * creation time automatically shares.
		 */
		inet->sport = htons(inet->num);
		sk->sk_prot->hash(sk);
	}
	if (sk->sk_prot->init) {
		err = sk->sk_prot->init(sk);
		if (err) {
			sk_common_release(sk);
			goto out;
		}
	}
out:
	return err;
out_rcu_unlock:
	rcu_read_unlock();
	goto out;
}