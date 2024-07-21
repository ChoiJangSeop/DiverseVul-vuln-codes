static int sco_send_frame(struct sock *sk, void *buf, int len,
			  unsigned int msg_flags)
{
	struct sco_conn *conn = sco_pi(sk)->conn;
	struct sk_buff *skb;
	int err;

	/* Check outgoing MTU */
	if (len > conn->mtu)
		return -EINVAL;

	BT_DBG("sk %p len %d", sk, len);

	skb = bt_skb_send_alloc(sk, len, msg_flags & MSG_DONTWAIT, &err);
	if (!skb)
		return err;

	memcpy(skb_put(skb, len), buf, len);
	hci_send_sco(conn->hcon, skb);

	return len;
}