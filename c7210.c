int dccp_recvmsg(struct kiocb *iocb, struct sock *sk, struct msghdr *msg,
		 size_t len, int nonblock, int flags, int *addr_len)
{
	const struct dccp_hdr *dh;
	long timeo;

	lock_sock(sk);

	if (sk->sk_state == DCCP_LISTEN) {
		len = -ENOTCONN;
		goto out;
	}

	timeo = sock_rcvtimeo(sk, nonblock);

	do {
		struct sk_buff *skb = skb_peek(&sk->sk_receive_queue);

		if (skb == NULL)
			goto verify_sock_status;

		dh = dccp_hdr(skb);

		switch (dh->dccph_type) {
		case DCCP_PKT_DATA:
		case DCCP_PKT_DATAACK:
			goto found_ok_skb;

		case DCCP_PKT_CLOSE:
		case DCCP_PKT_CLOSEREQ:
			if (!(flags & MSG_PEEK))
				dccp_finish_passive_close(sk);
			/* fall through */
		case DCCP_PKT_RESET:
			dccp_pr_debug("found fin (%s) ok!\n",
				      dccp_packet_name(dh->dccph_type));
			len = 0;
			goto found_fin_ok;
		default:
			dccp_pr_debug("packet_type=%s\n",
				      dccp_packet_name(dh->dccph_type));
			sk_eat_skb(sk, skb, false);
		}
verify_sock_status:
		if (sock_flag(sk, SOCK_DONE)) {
			len = 0;
			break;
		}

		if (sk->sk_err) {
			len = sock_error(sk);
			break;
		}

		if (sk->sk_shutdown & RCV_SHUTDOWN) {
			len = 0;
			break;
		}

		if (sk->sk_state == DCCP_CLOSED) {
			if (!sock_flag(sk, SOCK_DONE)) {
				/* This occurs when user tries to read
				 * from never connected socket.
				 */
				len = -ENOTCONN;
				break;
			}
			len = 0;
			break;
		}

		if (!timeo) {
			len = -EAGAIN;
			break;
		}

		if (signal_pending(current)) {
			len = sock_intr_errno(timeo);
			break;
		}

		sk_wait_data(sk, &timeo);
		continue;
	found_ok_skb:
		if (len > skb->len)
			len = skb->len;
		else if (len < skb->len)
			msg->msg_flags |= MSG_TRUNC;

		if (skb_copy_datagram_iovec(skb, 0, msg->msg_iov, len)) {
			/* Exception. Bailout! */
			len = -EFAULT;
			break;
		}
		if (flags & MSG_TRUNC)
			len = skb->len;
	found_fin_ok:
		if (!(flags & MSG_PEEK))
			sk_eat_skb(sk, skb, false);
		break;
	} while (1);
out:
	release_sock(sk);
	return len;
}