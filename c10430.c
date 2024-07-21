static void hci_loglink_complete_evt(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct hci_ev_logical_link_complete *ev = (void *) skb->data;
	struct hci_conn *hcon;
	struct hci_chan *hchan;
	struct amp_mgr *mgr;

	BT_DBG("%s log_handle 0x%4.4x phy_handle 0x%2.2x status 0x%2.2x",
	       hdev->name, le16_to_cpu(ev->handle), ev->phy_handle,
	       ev->status);

	hcon = hci_conn_hash_lookup_handle(hdev, ev->phy_handle);
	if (!hcon)
		return;

	/* Create AMP hchan */
	hchan = hci_chan_create(hcon);
	if (!hchan)
		return;

	hchan->handle = le16_to_cpu(ev->handle);

	BT_DBG("hcon %p mgr %p hchan %p", hcon, hcon->amp_mgr, hchan);

	mgr = hcon->amp_mgr;
	if (mgr && mgr->bredr_chan) {
		struct l2cap_chan *bredr_chan = mgr->bredr_chan;

		l2cap_chan_lock(bredr_chan);

		bredr_chan->conn->mtu = hdev->block_mtu;
		l2cap_logical_cfm(bredr_chan, hchan, 0);
		hci_conn_hold(hcon);

		l2cap_chan_unlock(bredr_chan);
	}
}