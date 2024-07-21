static int udpv6_sendmsg(struct kiocb *iocb, struct sock *sk, 
		  struct msghdr *msg, size_t len)
{
	struct ipv6_txoptions opt_space;
	struct udp_sock *up = udp_sk(sk);
	struct inet_sock *inet = inet_sk(sk);
	struct ipv6_pinfo *np = inet6_sk(sk);
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) msg->msg_name;
	struct in6_addr *daddr, *final_p = NULL, final;
	struct ipv6_txoptions *opt = NULL;
	struct ip6_flowlabel *flowlabel = NULL;
	struct flowi fl;
	struct dst_entry *dst;
	int addr_len = msg->msg_namelen;
	int ulen = len;
	int hlimit = -1;
	int tclass = -1;
	int corkreq = up->corkflag || msg->msg_flags&MSG_MORE;
	int err;
	int connected = 0;

	/* destination address check */
	if (sin6) {
		if (addr_len < offsetof(struct sockaddr, sa_data))
			return -EINVAL;

		switch (sin6->sin6_family) {
		case AF_INET6:
			if (addr_len < SIN6_LEN_RFC2133)
				return -EINVAL;
			daddr = &sin6->sin6_addr;
			break;
		case AF_INET:
			goto do_udp_sendmsg;
		case AF_UNSPEC:
			msg->msg_name = sin6 = NULL;
			msg->msg_namelen = addr_len = 0;
			daddr = NULL;
			break;
		default:
			return -EINVAL;
		}
	} else if (!up->pending) {
		if (sk->sk_state != TCP_ESTABLISHED)
			return -EDESTADDRREQ;
		daddr = &np->daddr;
	} else 
		daddr = NULL;

	if (daddr) {
		if (ipv6_addr_type(daddr) == IPV6_ADDR_MAPPED) {
			struct sockaddr_in sin;
			sin.sin_family = AF_INET;
			sin.sin_port = sin6 ? sin6->sin6_port : inet->dport;
			sin.sin_addr.s_addr = daddr->s6_addr32[3];
			msg->msg_name = &sin;
			msg->msg_namelen = sizeof(sin);
do_udp_sendmsg:
			if (__ipv6_only_sock(sk))
				return -ENETUNREACH;
			return udp_sendmsg(iocb, sk, msg, len);
		}
	}

	if (up->pending == AF_INET)
		return udp_sendmsg(iocb, sk, msg, len);

	/* Rough check on arithmetic overflow,
	   better check is made in ip6_build_xmit
	   */
	if (len > INT_MAX - sizeof(struct udphdr))
		return -EMSGSIZE;
	
	if (up->pending) {
		/*
		 * There are pending frames.
		 * The socket lock must be held while it's corked.
		 */
		lock_sock(sk);
		if (likely(up->pending)) {
			if (unlikely(up->pending != AF_INET6)) {
				release_sock(sk);
				return -EAFNOSUPPORT;
			}
			dst = NULL;
			goto do_append_data;
		}
		release_sock(sk);
	}
	ulen += sizeof(struct udphdr);

	memset(&fl, 0, sizeof(fl));

	if (sin6) {
		if (sin6->sin6_port == 0)
			return -EINVAL;

		fl.fl_ip_dport = sin6->sin6_port;
		daddr = &sin6->sin6_addr;

		if (np->sndflow) {
			fl.fl6_flowlabel = sin6->sin6_flowinfo&IPV6_FLOWINFO_MASK;
			if (fl.fl6_flowlabel&IPV6_FLOWLABEL_MASK) {
				flowlabel = fl6_sock_lookup(sk, fl.fl6_flowlabel);
				if (flowlabel == NULL)
					return -EINVAL;
				daddr = &flowlabel->dst;
			}
		}

		/*
		 * Otherwise it will be difficult to maintain
		 * sk->sk_dst_cache.
		 */
		if (sk->sk_state == TCP_ESTABLISHED &&
		    ipv6_addr_equal(daddr, &np->daddr))
			daddr = &np->daddr;

		if (addr_len >= sizeof(struct sockaddr_in6) &&
		    sin6->sin6_scope_id &&
		    ipv6_addr_type(daddr)&IPV6_ADDR_LINKLOCAL)
			fl.oif = sin6->sin6_scope_id;
	} else {
		if (sk->sk_state != TCP_ESTABLISHED)
			return -EDESTADDRREQ;

		fl.fl_ip_dport = inet->dport;
		daddr = &np->daddr;
		fl.fl6_flowlabel = np->flow_label;
		connected = 1;
	}

	if (!fl.oif)
		fl.oif = sk->sk_bound_dev_if;

	if (msg->msg_controllen) {
		opt = &opt_space;
		memset(opt, 0, sizeof(struct ipv6_txoptions));
		opt->tot_len = sizeof(*opt);

		err = datagram_send_ctl(msg, &fl, opt, &hlimit, &tclass);
		if (err < 0) {
			fl6_sock_release(flowlabel);
			return err;
		}
		if ((fl.fl6_flowlabel&IPV6_FLOWLABEL_MASK) && !flowlabel) {
			flowlabel = fl6_sock_lookup(sk, fl.fl6_flowlabel);
			if (flowlabel == NULL)
				return -EINVAL;
		}
		if (!(opt->opt_nflen|opt->opt_flen))
			opt = NULL;
		connected = 0;
	}
	if (opt == NULL)
		opt = np->opt;
	if (flowlabel)
		opt = fl6_merge_options(&opt_space, flowlabel, opt);
	opt = ipv6_fixup_options(&opt_space, opt);

	fl.proto = IPPROTO_UDP;
	ipv6_addr_copy(&fl.fl6_dst, daddr);
	if (ipv6_addr_any(&fl.fl6_src) && !ipv6_addr_any(&np->saddr))
		ipv6_addr_copy(&fl.fl6_src, &np->saddr);
	fl.fl_ip_sport = inet->sport;
	
	/* merge ip6_build_xmit from ip6_output */
	if (opt && opt->srcrt) {
		struct rt0_hdr *rt0 = (struct rt0_hdr *) opt->srcrt;
		ipv6_addr_copy(&final, &fl.fl6_dst);
		ipv6_addr_copy(&fl.fl6_dst, rt0->addr);
		final_p = &final;
		connected = 0;
	}

	if (!fl.oif && ipv6_addr_is_multicast(&fl.fl6_dst)) {
		fl.oif = np->mcast_oif;
		connected = 0;
	}

	security_sk_classify_flow(sk, &fl);

	err = ip6_sk_dst_lookup(sk, &dst, &fl);
	if (err)
		goto out;
	if (final_p)
		ipv6_addr_copy(&fl.fl6_dst, final_p);

	if ((err = xfrm_lookup(&dst, &fl, sk, 0)) < 0)
		goto out;

	if (hlimit < 0) {
		if (ipv6_addr_is_multicast(&fl.fl6_dst))
			hlimit = np->mcast_hops;
		else
			hlimit = np->hop_limit;
		if (hlimit < 0)
			hlimit = dst_metric(dst, RTAX_HOPLIMIT);
		if (hlimit < 0)
			hlimit = ipv6_get_hoplimit(dst->dev);
	}

	if (tclass < 0) {
		tclass = np->tclass;
		if (tclass < 0)
			tclass = 0;
	}

	if (msg->msg_flags&MSG_CONFIRM)
		goto do_confirm;
back_from_confirm:

	lock_sock(sk);
	if (unlikely(up->pending)) {
		/* The socket is already corked while preparing it. */
		/* ... which is an evident application bug. --ANK */
		release_sock(sk);

		LIMIT_NETDEBUG(KERN_DEBUG "udp cork app bug 2\n");
		err = -EINVAL;
		goto out;
	}

	up->pending = AF_INET6;

do_append_data:
	up->len += ulen;
	err = ip6_append_data(sk, ip_generic_getfrag, msg->msg_iov, ulen,
		sizeof(struct udphdr), hlimit, tclass, opt, &fl,
		(struct rt6_info*)dst,
		corkreq ? msg->msg_flags|MSG_MORE : msg->msg_flags);
	if (err)
		udp_v6_flush_pending_frames(sk);
	else if (!corkreq)
		err = udp_v6_push_pending_frames(sk, up);

	if (dst) {
		if (connected) {
			ip6_dst_store(sk, dst,
				      ipv6_addr_equal(&fl.fl6_dst, &np->daddr) ?
				      &np->daddr : NULL,
#ifdef CONFIG_IPV6_SUBTREES
				      ipv6_addr_equal(&fl.fl6_src, &np->saddr) ?
				      &np->saddr :
#endif
				      NULL);
		} else {
			dst_release(dst);
		}
	}

	if (err > 0)
		err = np->recverr ? net_xmit_errno(err) : 0;
	release_sock(sk);
out:
	fl6_sock_release(flowlabel);
	if (!err) {
		UDP6_INC_STATS_USER(UDP_MIB_OUTDATAGRAMS);
		return len;
	}
	/*
	 * ENOBUFS = no kernel mem, SOCK_NOSPACE = no sndbuf space.  Reporting
	 * ENOBUFS might not be good (it's not tunable per se), but otherwise
	 * we don't have a good statistic (IpOutDiscards but it can be too many
	 * things).  We could add another new stat but at least for now that
	 * seems like overkill.
	 */
	if (err == -ENOBUFS || test_bit(SOCK_NOSPACE, &sk->sk_socket->flags)) {
		UDP6_INC_STATS_USER(UDP_MIB_SNDBUFERRORS);
	}
	return err;

do_confirm:
	dst_confirm(dst);
	if (!(msg->msg_flags&MSG_PROBE) || len)
		goto back_from_confirm;
	err = 0;
	goto out;
}