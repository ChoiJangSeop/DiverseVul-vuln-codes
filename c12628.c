static int sco_send_frame(struct sock *sk, struct msghdr *msg, int len)
{
	struct sco_conn *conn = sco_pi(sk)->conn;
	struct sk_buff *skb;
	int err;

	/* Check outgoing MTU */
	if (len > conn->mtu)
		return -EINVAL;

	BT_DBG("sk %p len %d", sk, len);

	skb = bt_skb_send_alloc(sk, len, msg->msg_flags & MSG_DONTWAIT, &err);
	if (!skb)
		return err;

	if (memcpy_from_msg(skb_put(skb, len), msg, len)) {
		kfree_skb(skb);
		return -EFAULT;
	}

	hci_send_sco(conn->hcon, skb);

	return len;
}