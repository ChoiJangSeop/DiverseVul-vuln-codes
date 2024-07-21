static int ax25_bind(struct socket *sock, struct sockaddr *uaddr, int addr_len)
{
	struct sock *sk = sock->sk;
	struct full_sockaddr_ax25 *addr = (struct full_sockaddr_ax25 *)uaddr;
	ax25_dev *ax25_dev = NULL;
	ax25_uid_assoc *user;
	ax25_address call;
	ax25_cb *ax25;
	int err = 0;

	if (addr_len != sizeof(struct sockaddr_ax25) &&
	    addr_len != sizeof(struct full_sockaddr_ax25))
		/* support for old structure may go away some time
		 * ax25_bind(): uses old (6 digipeater) socket structure.
		 */
		if ((addr_len < sizeof(struct sockaddr_ax25) + sizeof(ax25_address) * 6) ||
		    (addr_len > sizeof(struct full_sockaddr_ax25)))
			return -EINVAL;

	if (addr->fsa_ax25.sax25_family != AF_AX25)
		return -EINVAL;

	user = ax25_findbyuid(current_euid());
	if (user) {
		call = user->call;
		ax25_uid_put(user);
	} else {
		if (ax25_uid_policy && !capable(CAP_NET_ADMIN))
			return -EACCES;

		call = addr->fsa_ax25.sax25_call;
	}

	lock_sock(sk);

	ax25 = sk_to_ax25(sk);
	if (!sock_flag(sk, SOCK_ZAPPED)) {
		err = -EINVAL;
		goto out;
	}

	ax25->source_addr = call;

	/*
	 * User already set interface with SO_BINDTODEVICE
	 */
	if (ax25->ax25_dev != NULL)
		goto done;

	if (addr_len > sizeof(struct sockaddr_ax25) && addr->fsa_ax25.sax25_ndigis == 1) {
		if (ax25cmp(&addr->fsa_digipeater[0], &null_ax25_address) != 0 &&
		    (ax25_dev = ax25_addr_ax25dev(&addr->fsa_digipeater[0])) == NULL) {
			err = -EADDRNOTAVAIL;
			goto out;
		}
	} else {
		if ((ax25_dev = ax25_addr_ax25dev(&addr->fsa_ax25.sax25_call)) == NULL) {
			err = -EADDRNOTAVAIL;
			goto out;
		}
	}

	if (ax25_dev != NULL)
		ax25_fillin_cb(ax25, ax25_dev);

done:
	ax25_cb_add(ax25);
	sock_reset_flag(sk, SOCK_ZAPPED);

out:
	release_sock(sk);

	return err;
}