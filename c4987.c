packet_setsockopt(struct socket *sock, int level, int optname, char __user *optval, unsigned int optlen)
{
	struct sock *sk = sock->sk;
	struct packet_sock *po = pkt_sk(sk);
	int ret;

	if (level != SOL_PACKET)
		return -ENOPROTOOPT;

	switch (optname) {
	case PACKET_ADD_MEMBERSHIP:
	case PACKET_DROP_MEMBERSHIP:
	{
		struct packet_mreq_max mreq;
		int len = optlen;
		memset(&mreq, 0, sizeof(mreq));
		if (len < sizeof(struct packet_mreq))
			return -EINVAL;
		if (len > sizeof(mreq))
			len = sizeof(mreq);
		if (copy_from_user(&mreq, optval, len))
			return -EFAULT;
		if (len < (mreq.mr_alen + offsetof(struct packet_mreq, mr_address)))
			return -EINVAL;
		if (optname == PACKET_ADD_MEMBERSHIP)
			ret = packet_mc_add(sk, &mreq);
		else
			ret = packet_mc_drop(sk, &mreq);
		return ret;
	}

	case PACKET_RX_RING:
	case PACKET_TX_RING:
	{
		union tpacket_req_u req_u;
		int len;

		switch (po->tp_version) {
		case TPACKET_V1:
		case TPACKET_V2:
			len = sizeof(req_u.req);
			break;
		case TPACKET_V3:
		default:
			len = sizeof(req_u.req3);
			break;
		}
		if (optlen < len)
			return -EINVAL;
		if (copy_from_user(&req_u.req, optval, len))
			return -EFAULT;
		return packet_set_ring(sk, &req_u, 0,
			optname == PACKET_TX_RING);
	}
	case PACKET_COPY_THRESH:
	{
		int val;

		if (optlen != sizeof(val))
			return -EINVAL;
		if (copy_from_user(&val, optval, sizeof(val)))
			return -EFAULT;

		pkt_sk(sk)->copy_thresh = val;
		return 0;
	}
	case PACKET_VERSION:
	{
		int val;

		if (optlen != sizeof(val))
			return -EINVAL;
		if (copy_from_user(&val, optval, sizeof(val)))
			return -EFAULT;
		switch (val) {
		case TPACKET_V1:
		case TPACKET_V2:
		case TPACKET_V3:
			break;
		default:
			return -EINVAL;
		}
		lock_sock(sk);
		if (po->rx_ring.pg_vec || po->tx_ring.pg_vec) {
			ret = -EBUSY;
		} else {
			po->tp_version = val;
			ret = 0;
		}
		release_sock(sk);
		return ret;
	}
	case PACKET_RESERVE:
	{
		unsigned int val;

		if (optlen != sizeof(val))
			return -EINVAL;
		if (po->rx_ring.pg_vec || po->tx_ring.pg_vec)
			return -EBUSY;
		if (copy_from_user(&val, optval, sizeof(val)))
			return -EFAULT;
		po->tp_reserve = val;
		return 0;
	}
	case PACKET_LOSS:
	{
		unsigned int val;

		if (optlen != sizeof(val))
			return -EINVAL;
		if (po->rx_ring.pg_vec || po->tx_ring.pg_vec)
			return -EBUSY;
		if (copy_from_user(&val, optval, sizeof(val)))
			return -EFAULT;
		po->tp_loss = !!val;
		return 0;
	}
	case PACKET_AUXDATA:
	{
		int val;

		if (optlen < sizeof(val))
			return -EINVAL;
		if (copy_from_user(&val, optval, sizeof(val)))
			return -EFAULT;

		po->auxdata = !!val;
		return 0;
	}
	case PACKET_ORIGDEV:
	{
		int val;

		if (optlen < sizeof(val))
			return -EINVAL;
		if (copy_from_user(&val, optval, sizeof(val)))
			return -EFAULT;

		po->origdev = !!val;
		return 0;
	}
	case PACKET_VNET_HDR:
	{
		int val;

		if (sock->type != SOCK_RAW)
			return -EINVAL;
		if (po->rx_ring.pg_vec || po->tx_ring.pg_vec)
			return -EBUSY;
		if (optlen < sizeof(val))
			return -EINVAL;
		if (copy_from_user(&val, optval, sizeof(val)))
			return -EFAULT;

		po->has_vnet_hdr = !!val;
		return 0;
	}
	case PACKET_TIMESTAMP:
	{
		int val;

		if (optlen != sizeof(val))
			return -EINVAL;
		if (copy_from_user(&val, optval, sizeof(val)))
			return -EFAULT;

		po->tp_tstamp = val;
		return 0;
	}
	case PACKET_FANOUT:
	{
		int val;

		if (optlen != sizeof(val))
			return -EINVAL;
		if (copy_from_user(&val, optval, sizeof(val)))
			return -EFAULT;

		return fanout_add(sk, val & 0xffff, val >> 16);
	}
	case PACKET_FANOUT_DATA:
	{
		if (!po->fanout)
			return -EINVAL;

		return fanout_set_data(po, optval, optlen);
	}
	case PACKET_TX_HAS_OFF:
	{
		unsigned int val;

		if (optlen != sizeof(val))
			return -EINVAL;
		if (po->rx_ring.pg_vec || po->tx_ring.pg_vec)
			return -EBUSY;
		if (copy_from_user(&val, optval, sizeof(val)))
			return -EFAULT;
		po->tp_tx_has_off = !!val;
		return 0;
	}
	case PACKET_QDISC_BYPASS:
	{
		int val;

		if (optlen != sizeof(val))
			return -EINVAL;
		if (copy_from_user(&val, optval, sizeof(val)))
			return -EFAULT;

		po->xmit = val ? packet_direct_xmit : dev_queue_xmit;
		return 0;
	}
	default:
		return -ENOPROTOOPT;
	}
}