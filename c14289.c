static int l2cap_sock_getname(struct socket *sock, struct sockaddr *addr, int *len, int peer)
{
	struct sockaddr_l2 *la = (struct sockaddr_l2 *) addr;
	struct sock *sk = sock->sk;
	struct l2cap_chan *chan = l2cap_pi(sk)->chan;

	BT_DBG("sock %p, sk %p", sock, sk);

	addr->sa_family = AF_BLUETOOTH;
	*len = sizeof(struct sockaddr_l2);

	if (peer) {
		la->l2_psm = chan->psm;
		bacpy(&la->l2_bdaddr, &bt_sk(sk)->dst);
		la->l2_cid = cpu_to_le16(chan->dcid);
	} else {
		la->l2_psm = chan->sport;
		bacpy(&la->l2_bdaddr, &bt_sk(sk)->src);
		la->l2_cid = cpu_to_le16(chan->scid);
	}

	return 0;
}