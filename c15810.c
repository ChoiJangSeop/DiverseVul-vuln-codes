static int ccid3_hc_tx_getsockopt(struct sock *sk, const int optname, int len,
				  u32 __user *optval, int __user *optlen)
{
	const struct ccid3_hc_tx_sock *hc = ccid3_hc_tx_sk(sk);
	struct tfrc_tx_info tfrc;
	const void *val;

	switch (optname) {
	case DCCP_SOCKOPT_CCID_TX_INFO:
		if (len < sizeof(tfrc))
			return -EINVAL;
		tfrc.tfrctx_x	   = hc->tx_x;
		tfrc.tfrctx_x_recv = hc->tx_x_recv;
		tfrc.tfrctx_x_calc = hc->tx_x_calc;
		tfrc.tfrctx_rtt	   = hc->tx_rtt;
		tfrc.tfrctx_p	   = hc->tx_p;
		tfrc.tfrctx_rto	   = hc->tx_t_rto;
		tfrc.tfrctx_ipi	   = hc->tx_t_ipi;
		len = sizeof(tfrc);
		val = &tfrc;
		break;
	default:
		return -ENOPROTOOPT;
	}

	if (put_user(len, optlen) || copy_to_user(optval, val, len))
		return -EFAULT;

	return 0;
}