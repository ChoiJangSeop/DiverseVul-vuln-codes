static int sco_sock_sendmsg(struct socket *sock, struct msghdr *msg,
			    size_t len)
{
	struct sock *sk = sock->sk;
	int err;

	BT_DBG("sock %p, sk %p", sock, sk);

	err = sock_error(sk);
	if (err)
		return err;

	if (msg->msg_flags & MSG_OOB)
		return -EOPNOTSUPP;

	lock_sock(sk);

	if (sk->sk_state == BT_CONNECTED)
		err = sco_send_frame(sk, msg, len);
	else
		err = -ENOTCONN;

	release_sock(sk);
	return err;
}