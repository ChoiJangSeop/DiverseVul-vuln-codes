static ssize_t macvtap_get_user(struct macvtap_queue *q, struct msghdr *m,
				const struct iovec *iv, unsigned long total_len,
				size_t count, int noblock)
{
	struct sk_buff *skb;
	struct macvlan_dev *vlan;
	unsigned long len = total_len;
	int err;
	struct virtio_net_hdr vnet_hdr = { 0 };
	int vnet_hdr_len = 0;
	int copylen;
	bool zerocopy = false;

	if (q->flags & IFF_VNET_HDR) {
		vnet_hdr_len = q->vnet_hdr_sz;

		err = -EINVAL;
		if (len < vnet_hdr_len)
			goto err;
		len -= vnet_hdr_len;

		err = memcpy_fromiovecend((void *)&vnet_hdr, iv, 0,
					   sizeof(vnet_hdr));
		if (err < 0)
			goto err;
		if ((vnet_hdr.flags & VIRTIO_NET_HDR_F_NEEDS_CSUM) &&
		     vnet_hdr.csum_start + vnet_hdr.csum_offset + 2 >
							vnet_hdr.hdr_len)
			vnet_hdr.hdr_len = vnet_hdr.csum_start +
						vnet_hdr.csum_offset + 2;
		err = -EINVAL;
		if (vnet_hdr.hdr_len > len)
			goto err;
	}

	err = -EINVAL;
	if (unlikely(len < ETH_HLEN))
		goto err;

	if (m && m->msg_control && sock_flag(&q->sk, SOCK_ZEROCOPY))
		zerocopy = true;

	if (zerocopy) {
		/* There are 256 bytes to be copied in skb, so there is enough
		 * room for skb expand head in case it is used.
		 * The rest buffer is mapped from userspace.
		 */
		copylen = vnet_hdr.hdr_len;
		if (!copylen)
			copylen = GOODCOPY_LEN;
	} else
		copylen = len;

	skb = macvtap_alloc_skb(&q->sk, NET_IP_ALIGN, copylen,
				vnet_hdr.hdr_len, noblock, &err);
	if (!skb)
		goto err;

	if (zerocopy)
		err = zerocopy_sg_from_iovec(skb, iv, vnet_hdr_len, count);
	else
		err = skb_copy_datagram_from_iovec(skb, 0, iv, vnet_hdr_len,
						   len);
	if (err)
		goto err_kfree;

	skb_set_network_header(skb, ETH_HLEN);
	skb_reset_mac_header(skb);
	skb->protocol = eth_hdr(skb)->h_proto;

	if (vnet_hdr_len) {
		err = macvtap_skb_from_vnet_hdr(skb, &vnet_hdr);
		if (err)
			goto err_kfree;
	}

	rcu_read_lock_bh();
	vlan = rcu_dereference_bh(q->vlan);
	/* copy skb_ubuf_info for callback when skb has no error */
	if (zerocopy) {
		skb_shinfo(skb)->destructor_arg = m->msg_control;
		skb_shinfo(skb)->tx_flags |= SKBTX_DEV_ZEROCOPY;
	}
	if (vlan)
		macvlan_start_xmit(skb, vlan->dev);
	else
		kfree_skb(skb);
	rcu_read_unlock_bh();

	return total_len;

err_kfree:
	kfree_skb(skb);

err:
	rcu_read_lock_bh();
	vlan = rcu_dereference_bh(q->vlan);
	if (vlan)
		vlan->dev->stats.tx_dropped++;
	rcu_read_unlock_bh();

	return err;
}