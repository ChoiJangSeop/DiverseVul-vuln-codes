static int do_ipv6_getsockopt(struct sock *sk, int level, int optname,
		    char __user *optval, int __user *optlen, unsigned int flags)
{
	struct ipv6_pinfo *np = inet6_sk(sk);
	int len;
	int val;

	if (ip6_mroute_opt(optname))
		return ip6_mroute_getsockopt(sk, optname, optval, optlen);

	if (get_user(len, optlen))
		return -EFAULT;
	switch (optname) {
	case IPV6_ADDRFORM:
		if (sk->sk_protocol != IPPROTO_UDP &&
		    sk->sk_protocol != IPPROTO_UDPLITE &&
		    sk->sk_protocol != IPPROTO_TCP)
			return -ENOPROTOOPT;
		if (sk->sk_state != TCP_ESTABLISHED)
			return -ENOTCONN;
		val = sk->sk_family;
		break;
	case MCAST_MSFILTER:
	{
		struct group_filter gsf;
		int err;

		if (len < GROUP_FILTER_SIZE(0))
			return -EINVAL;
		if (copy_from_user(&gsf, optval, GROUP_FILTER_SIZE(0)))
			return -EFAULT;
		if (gsf.gf_group.ss_family != AF_INET6)
			return -EADDRNOTAVAIL;
		lock_sock(sk);
		err = ip6_mc_msfget(sk, &gsf,
			(struct group_filter __user *)optval, optlen);
		release_sock(sk);
		return err;
	}

	case IPV6_2292PKTOPTIONS:
	{
		struct msghdr msg;
		struct sk_buff *skb;

		if (sk->sk_type != SOCK_STREAM)
			return -ENOPROTOOPT;

		msg.msg_control = optval;
		msg.msg_controllen = len;
		msg.msg_flags = flags;

		lock_sock(sk);
		skb = np->pktoptions;
		if (skb)
			ip6_datagram_recv_ctl(sk, &msg, skb);
		release_sock(sk);
		if (!skb) {
			if (np->rxopt.bits.rxinfo) {
				struct in6_pktinfo src_info;
				src_info.ipi6_ifindex = np->mcast_oif ? np->mcast_oif :
					np->sticky_pktinfo.ipi6_ifindex;
				src_info.ipi6_addr = np->mcast_oif ? sk->sk_v6_daddr : np->sticky_pktinfo.ipi6_addr;
				put_cmsg(&msg, SOL_IPV6, IPV6_PKTINFO, sizeof(src_info), &src_info);
			}
			if (np->rxopt.bits.rxhlim) {
				int hlim = np->mcast_hops;
				put_cmsg(&msg, SOL_IPV6, IPV6_HOPLIMIT, sizeof(hlim), &hlim);
			}
			if (np->rxopt.bits.rxtclass) {
				int tclass = (int)ip6_tclass(np->rcv_flowinfo);

				put_cmsg(&msg, SOL_IPV6, IPV6_TCLASS, sizeof(tclass), &tclass);
			}
			if (np->rxopt.bits.rxoinfo) {
				struct in6_pktinfo src_info;
				src_info.ipi6_ifindex = np->mcast_oif ? np->mcast_oif :
					np->sticky_pktinfo.ipi6_ifindex;
				src_info.ipi6_addr = np->mcast_oif ? sk->sk_v6_daddr :
								     np->sticky_pktinfo.ipi6_addr;
				put_cmsg(&msg, SOL_IPV6, IPV6_2292PKTINFO, sizeof(src_info), &src_info);
			}
			if (np->rxopt.bits.rxohlim) {
				int hlim = np->mcast_hops;
				put_cmsg(&msg, SOL_IPV6, IPV6_2292HOPLIMIT, sizeof(hlim), &hlim);
			}
			if (np->rxopt.bits.rxflow) {
				__be32 flowinfo = np->rcv_flowinfo;

				put_cmsg(&msg, SOL_IPV6, IPV6_FLOWINFO, sizeof(flowinfo), &flowinfo);
			}
		}
		len -= msg.msg_controllen;
		return put_user(len, optlen);
	}
	case IPV6_MTU:
	{
		struct dst_entry *dst;

		val = 0;
		rcu_read_lock();
		dst = __sk_dst_get(sk);
		if (dst)
			val = dst_mtu(dst);
		rcu_read_unlock();
		if (!val)
			return -ENOTCONN;
		break;
	}

	case IPV6_V6ONLY:
		val = sk->sk_ipv6only;
		break;

	case IPV6_RECVPKTINFO:
		val = np->rxopt.bits.rxinfo;
		break;

	case IPV6_2292PKTINFO:
		val = np->rxopt.bits.rxoinfo;
		break;

	case IPV6_RECVHOPLIMIT:
		val = np->rxopt.bits.rxhlim;
		break;

	case IPV6_2292HOPLIMIT:
		val = np->rxopt.bits.rxohlim;
		break;

	case IPV6_RECVRTHDR:
		val = np->rxopt.bits.srcrt;
		break;

	case IPV6_2292RTHDR:
		val = np->rxopt.bits.osrcrt;
		break;

	case IPV6_HOPOPTS:
	case IPV6_RTHDRDSTOPTS:
	case IPV6_RTHDR:
	case IPV6_DSTOPTS:
	{

		lock_sock(sk);
		len = ipv6_getsockopt_sticky(sk, np->opt,
					     optname, optval, len);
		release_sock(sk);
		/* check if ipv6_getsockopt_sticky() returns err code */
		if (len < 0)
			return len;
		return put_user(len, optlen);
	}

	case IPV6_RECVHOPOPTS:
		val = np->rxopt.bits.hopopts;
		break;

	case IPV6_2292HOPOPTS:
		val = np->rxopt.bits.ohopopts;
		break;

	case IPV6_RECVDSTOPTS:
		val = np->rxopt.bits.dstopts;
		break;

	case IPV6_2292DSTOPTS:
		val = np->rxopt.bits.odstopts;
		break;

	case IPV6_TCLASS:
		val = np->tclass;
		break;

	case IPV6_RECVTCLASS:
		val = np->rxopt.bits.rxtclass;
		break;

	case IPV6_FLOWINFO:
		val = np->rxopt.bits.rxflow;
		break;

	case IPV6_RECVPATHMTU:
		val = np->rxopt.bits.rxpmtu;
		break;

	case IPV6_PATHMTU:
	{
		struct dst_entry *dst;
		struct ip6_mtuinfo mtuinfo;

		if (len < sizeof(mtuinfo))
			return -EINVAL;

		len = sizeof(mtuinfo);
		memset(&mtuinfo, 0, sizeof(mtuinfo));

		rcu_read_lock();
		dst = __sk_dst_get(sk);
		if (dst)
			mtuinfo.ip6m_mtu = dst_mtu(dst);
		rcu_read_unlock();
		if (!mtuinfo.ip6m_mtu)
			return -ENOTCONN;

		if (put_user(len, optlen))
			return -EFAULT;
		if (copy_to_user(optval, &mtuinfo, len))
			return -EFAULT;

		return 0;
	}

	case IPV6_TRANSPARENT:
		val = inet_sk(sk)->transparent;
		break;

	case IPV6_RECVORIGDSTADDR:
		val = np->rxopt.bits.rxorigdstaddr;
		break;

	case IPV6_UNICAST_HOPS:
	case IPV6_MULTICAST_HOPS:
	{
		struct dst_entry *dst;

		if (optname == IPV6_UNICAST_HOPS)
			val = np->hop_limit;
		else
			val = np->mcast_hops;

		if (val < 0) {
			rcu_read_lock();
			dst = __sk_dst_get(sk);
			if (dst)
				val = ip6_dst_hoplimit(dst);
			rcu_read_unlock();
		}

		if (val < 0)
			val = sock_net(sk)->ipv6.devconf_all->hop_limit;
		break;
	}

	case IPV6_MULTICAST_LOOP:
		val = np->mc_loop;
		break;

	case IPV6_MULTICAST_IF:
		val = np->mcast_oif;
		break;

	case IPV6_UNICAST_IF:
		val = (__force int)htonl((__u32) np->ucast_oif);
		break;

	case IPV6_MTU_DISCOVER:
		val = np->pmtudisc;
		break;

	case IPV6_RECVERR:
		val = np->recverr;
		break;

	case IPV6_FLOWINFO_SEND:
		val = np->sndflow;
		break;

	case IPV6_FLOWLABEL_MGR:
	{
		struct in6_flowlabel_req freq;
		int flags;

		if (len < sizeof(freq))
			return -EINVAL;

		if (copy_from_user(&freq, optval, sizeof(freq)))
			return -EFAULT;

		if (freq.flr_action != IPV6_FL_A_GET)
			return -EINVAL;

		len = sizeof(freq);
		flags = freq.flr_flags;

		memset(&freq, 0, sizeof(freq));

		val = ipv6_flowlabel_opt_get(sk, &freq, flags);
		if (val < 0)
			return val;

		if (put_user(len, optlen))
			return -EFAULT;
		if (copy_to_user(optval, &freq, len))
			return -EFAULT;

		return 0;
	}

	case IPV6_ADDR_PREFERENCES:
		val = 0;

		if (np->srcprefs & IPV6_PREFER_SRC_TMP)
			val |= IPV6_PREFER_SRC_TMP;
		else if (np->srcprefs & IPV6_PREFER_SRC_PUBLIC)
			val |= IPV6_PREFER_SRC_PUBLIC;
		else {
			/* XXX: should we return system default? */
			val |= IPV6_PREFER_SRC_PUBTMP_DEFAULT;
		}

		if (np->srcprefs & IPV6_PREFER_SRC_COA)
			val |= IPV6_PREFER_SRC_COA;
		else
			val |= IPV6_PREFER_SRC_HOME;
		break;

	case IPV6_MINHOPCOUNT:
		val = np->min_hopcount;
		break;

	case IPV6_DONTFRAG:
		val = np->dontfrag;
		break;

	case IPV6_AUTOFLOWLABEL:
		val = np->autoflowlabel;
		break;

	default:
		return -ENOPROTOOPT;
	}
	len = min_t(unsigned int, sizeof(int), len);
	if (put_user(len, optlen))
		return -EFAULT;
	if (copy_to_user(optval, &val, len))
		return -EFAULT;
	return 0;
}