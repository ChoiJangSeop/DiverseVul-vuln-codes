static void brcmf_msgbuf_process_event(struct brcmf_msgbuf *msgbuf, void *buf)
{
	struct msgbuf_rx_event *event;
	u32 idx;
	u16 buflen;
	struct sk_buff *skb;
	struct brcmf_if *ifp;

	event = (struct msgbuf_rx_event *)buf;
	idx = le32_to_cpu(event->msg.request_id);
	buflen = le16_to_cpu(event->event_data_len);

	if (msgbuf->cur_eventbuf)
		msgbuf->cur_eventbuf--;
	brcmf_msgbuf_rxbuf_event_post(msgbuf);

	skb = brcmf_msgbuf_get_pktid(msgbuf->drvr->bus_if->dev,
				     msgbuf->rx_pktids, idx);
	if (!skb)
		return;

	if (msgbuf->rx_dataoffset)
		skb_pull(skb, msgbuf->rx_dataoffset);

	skb_trim(skb, buflen);

	ifp = brcmf_get_ifp(msgbuf->drvr, event->msg.ifidx);
	if (!ifp || !ifp->ndev) {
		brcmf_err("Received pkt for invalid ifidx %d\n",
			  event->msg.ifidx);
		goto exit;
	}

	skb->protocol = eth_type_trans(skb, ifp->ndev);

	brcmf_fweh_process_skb(ifp->drvr, skb);

exit:
	brcmu_pkt_buf_free_skb(skb);
}