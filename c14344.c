static void l2cap_sock_close_cb(struct l2cap_chan *chan)
{
	struct sock *sk = chan->data;

	l2cap_sock_kill(sk);
}