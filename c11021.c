int sock_getsockopt(struct socket *sock, int level, int optname,
		    char __user *optval, int __user *optlen)
{
	struct sock *sk = sock->sk;

	union {
		int val;
		u64 val64;
		unsigned long ulval;
		struct linger ling;
		struct old_timeval32 tm32;
		struct __kernel_old_timeval tm;
		struct  __kernel_sock_timeval stm;
		struct sock_txtime txtime;
		struct so_timestamping timestamping;
	} v;

	int lv = sizeof(int);
	int len;

	if (get_user(len, optlen))
		return -EFAULT;
	if (len < 0)
		return -EINVAL;

	memset(&v, 0, sizeof(v));

	switch (optname) {
	case SO_DEBUG:
		v.val = sock_flag(sk, SOCK_DBG);
		break;

	case SO_DONTROUTE:
		v.val = sock_flag(sk, SOCK_LOCALROUTE);
		break;

	case SO_BROADCAST:
		v.val = sock_flag(sk, SOCK_BROADCAST);
		break;

	case SO_SNDBUF:
		v.val = sk->sk_sndbuf;
		break;

	case SO_RCVBUF:
		v.val = sk->sk_rcvbuf;
		break;

	case SO_REUSEADDR:
		v.val = sk->sk_reuse;
		break;

	case SO_REUSEPORT:
		v.val = sk->sk_reuseport;
		break;

	case SO_KEEPALIVE:
		v.val = sock_flag(sk, SOCK_KEEPOPEN);
		break;

	case SO_TYPE:
		v.val = sk->sk_type;
		break;

	case SO_PROTOCOL:
		v.val = sk->sk_protocol;
		break;

	case SO_DOMAIN:
		v.val = sk->sk_family;
		break;

	case SO_ERROR:
		v.val = -sock_error(sk);
		if (v.val == 0)
			v.val = xchg(&sk->sk_err_soft, 0);
		break;

	case SO_OOBINLINE:
		v.val = sock_flag(sk, SOCK_URGINLINE);
		break;

	case SO_NO_CHECK:
		v.val = sk->sk_no_check_tx;
		break;

	case SO_PRIORITY:
		v.val = sk->sk_priority;
		break;

	case SO_LINGER:
		lv		= sizeof(v.ling);
		v.ling.l_onoff	= sock_flag(sk, SOCK_LINGER);
		v.ling.l_linger	= sk->sk_lingertime / HZ;
		break;

	case SO_BSDCOMPAT:
		break;

	case SO_TIMESTAMP_OLD:
		v.val = sock_flag(sk, SOCK_RCVTSTAMP) &&
				!sock_flag(sk, SOCK_TSTAMP_NEW) &&
				!sock_flag(sk, SOCK_RCVTSTAMPNS);
		break;

	case SO_TIMESTAMPNS_OLD:
		v.val = sock_flag(sk, SOCK_RCVTSTAMPNS) && !sock_flag(sk, SOCK_TSTAMP_NEW);
		break;

	case SO_TIMESTAMP_NEW:
		v.val = sock_flag(sk, SOCK_RCVTSTAMP) && sock_flag(sk, SOCK_TSTAMP_NEW);
		break;

	case SO_TIMESTAMPNS_NEW:
		v.val = sock_flag(sk, SOCK_RCVTSTAMPNS) && sock_flag(sk, SOCK_TSTAMP_NEW);
		break;

	case SO_TIMESTAMPING_OLD:
		lv = sizeof(v.timestamping);
		v.timestamping.flags = sk->sk_tsflags;
		v.timestamping.bind_phc = sk->sk_bind_phc;
		break;

	case SO_RCVTIMEO_OLD:
	case SO_RCVTIMEO_NEW:
		lv = sock_get_timeout(sk->sk_rcvtimeo, &v, SO_RCVTIMEO_OLD == optname);
		break;

	case SO_SNDTIMEO_OLD:
	case SO_SNDTIMEO_NEW:
		lv = sock_get_timeout(sk->sk_sndtimeo, &v, SO_SNDTIMEO_OLD == optname);
		break;

	case SO_RCVLOWAT:
		v.val = sk->sk_rcvlowat;
		break;

	case SO_SNDLOWAT:
		v.val = 1;
		break;

	case SO_PASSCRED:
		v.val = !!test_bit(SOCK_PASSCRED, &sock->flags);
		break;

	case SO_PEERCRED:
	{
		struct ucred peercred;
		if (len > sizeof(peercred))
			len = sizeof(peercred);
		cred_to_ucred(sk->sk_peer_pid, sk->sk_peer_cred, &peercred);
		if (copy_to_user(optval, &peercred, len))
			return -EFAULT;
		goto lenout;
	}

	case SO_PEERGROUPS:
	{
		int ret, n;

		if (!sk->sk_peer_cred)
			return -ENODATA;

		n = sk->sk_peer_cred->group_info->ngroups;
		if (len < n * sizeof(gid_t)) {
			len = n * sizeof(gid_t);
			return put_user(len, optlen) ? -EFAULT : -ERANGE;
		}
		len = n * sizeof(gid_t);

		ret = groups_to_user((gid_t __user *)optval,
				     sk->sk_peer_cred->group_info);
		if (ret)
			return ret;
		goto lenout;
	}

	case SO_PEERNAME:
	{
		char address[128];

		lv = sock->ops->getname(sock, (struct sockaddr *)address, 2);
		if (lv < 0)
			return -ENOTCONN;
		if (lv < len)
			return -EINVAL;
		if (copy_to_user(optval, address, len))
			return -EFAULT;
		goto lenout;
	}

	/* Dubious BSD thing... Probably nobody even uses it, but
	 * the UNIX standard wants it for whatever reason... -DaveM
	 */
	case SO_ACCEPTCONN:
		v.val = sk->sk_state == TCP_LISTEN;
		break;

	case SO_PASSSEC:
		v.val = !!test_bit(SOCK_PASSSEC, &sock->flags);
		break;

	case SO_PEERSEC:
		return security_socket_getpeersec_stream(sock, optval, optlen, len);

	case SO_MARK:
		v.val = sk->sk_mark;
		break;

	case SO_RXQ_OVFL:
		v.val = sock_flag(sk, SOCK_RXQ_OVFL);
		break;

	case SO_WIFI_STATUS:
		v.val = sock_flag(sk, SOCK_WIFI_STATUS);
		break;

	case SO_PEEK_OFF:
		if (!sock->ops->set_peek_off)
			return -EOPNOTSUPP;

		v.val = sk->sk_peek_off;
		break;
	case SO_NOFCS:
		v.val = sock_flag(sk, SOCK_NOFCS);
		break;

	case SO_BINDTODEVICE:
		return sock_getbindtodevice(sk, optval, optlen, len);

	case SO_GET_FILTER:
		len = sk_get_filter(sk, (struct sock_filter __user *)optval, len);
		if (len < 0)
			return len;

		goto lenout;

	case SO_LOCK_FILTER:
		v.val = sock_flag(sk, SOCK_FILTER_LOCKED);
		break;

	case SO_BPF_EXTENSIONS:
		v.val = bpf_tell_extensions();
		break;

	case SO_SELECT_ERR_QUEUE:
		v.val = sock_flag(sk, SOCK_SELECT_ERR_QUEUE);
		break;

#ifdef CONFIG_NET_RX_BUSY_POLL
	case SO_BUSY_POLL:
		v.val = sk->sk_ll_usec;
		break;
	case SO_PREFER_BUSY_POLL:
		v.val = READ_ONCE(sk->sk_prefer_busy_poll);
		break;
#endif

	case SO_MAX_PACING_RATE:
		if (sizeof(v.ulval) != sizeof(v.val) && len >= sizeof(v.ulval)) {
			lv = sizeof(v.ulval);
			v.ulval = sk->sk_max_pacing_rate;
		} else {
			/* 32bit version */
			v.val = min_t(unsigned long, sk->sk_max_pacing_rate, ~0U);
		}
		break;

	case SO_INCOMING_CPU:
		v.val = READ_ONCE(sk->sk_incoming_cpu);
		break;

	case SO_MEMINFO:
	{
		u32 meminfo[SK_MEMINFO_VARS];

		sk_get_meminfo(sk, meminfo);

		len = min_t(unsigned int, len, sizeof(meminfo));
		if (copy_to_user(optval, &meminfo, len))
			return -EFAULT;

		goto lenout;
	}

#ifdef CONFIG_NET_RX_BUSY_POLL
	case SO_INCOMING_NAPI_ID:
		v.val = READ_ONCE(sk->sk_napi_id);

		/* aggregate non-NAPI IDs down to 0 */
		if (v.val < MIN_NAPI_ID)
			v.val = 0;

		break;
#endif

	case SO_COOKIE:
		lv = sizeof(u64);
		if (len < lv)
			return -EINVAL;
		v.val64 = sock_gen_cookie(sk);
		break;

	case SO_ZEROCOPY:
		v.val = sock_flag(sk, SOCK_ZEROCOPY);
		break;

	case SO_TXTIME:
		lv = sizeof(v.txtime);
		v.txtime.clockid = sk->sk_clockid;
		v.txtime.flags |= sk->sk_txtime_deadline_mode ?
				  SOF_TXTIME_DEADLINE_MODE : 0;
		v.txtime.flags |= sk->sk_txtime_report_errors ?
				  SOF_TXTIME_REPORT_ERRORS : 0;
		break;

	case SO_BINDTOIFINDEX:
		v.val = sk->sk_bound_dev_if;
		break;

	case SO_NETNS_COOKIE:
		lv = sizeof(u64);
		if (len != lv)
			return -EINVAL;
		v.val64 = sock_net(sk)->net_cookie;
		break;

	case SO_BUF_LOCK:
		v.val = sk->sk_userlocks & SOCK_BUF_LOCK_MASK;
		break;

	default:
		/* We implement the SO_SNDLOWAT etc to not be settable
		 * (1003.1g 7).
		 */
		return -ENOPROTOOPT;
	}

	if (len > lv)
		len = lv;
	if (copy_to_user(optval, &v, len))
		return -EFAULT;
lenout:
	if (put_user(len, optlen))
		return -EFAULT;
	return 0;
}