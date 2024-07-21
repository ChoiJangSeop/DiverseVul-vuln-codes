static int do_dccp_getsockopt(struct sock *sk, int level, int optname,
		    char __user *optval, int __user *optlen)
{
	struct dccp_sock *dp;
	int val, len;

	if (get_user(len, optlen))
		return -EFAULT;

	if (len < sizeof(int))
		return -EINVAL;

	dp = dccp_sk(sk);

	switch (optname) {
	case DCCP_SOCKOPT_PACKET_SIZE:
		DCCP_WARN("sockopt(PACKET_SIZE) is deprecated: fix your app\n");
		return 0;
	case DCCP_SOCKOPT_SERVICE:
		return dccp_getsockopt_service(sk, len,
					       (__be32 __user *)optval, optlen);
	case DCCP_SOCKOPT_SEND_CSCOV:
		val = dp->dccps_pcslen;
		break;
	case DCCP_SOCKOPT_RECV_CSCOV:
		val = dp->dccps_pcrlen;
		break;
	case 128 ... 191:
		return ccid_hc_rx_getsockopt(dp->dccps_hc_rx_ccid, sk, optname,
					     len, (u32 __user *)optval, optlen);
	case 192 ... 255:
		return ccid_hc_tx_getsockopt(dp->dccps_hc_tx_ccid, sk, optname,
					     len, (u32 __user *)optval, optlen);
	default:
		return -ENOPROTOOPT;
	}

	if (put_user(len, optlen) || copy_to_user(optval, &val, len))
		return -EFAULT;

	return 0;
}