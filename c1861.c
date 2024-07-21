static int queue_userspace_packet(struct datapath *dp, struct sk_buff *skb,
				  const struct dp_upcall_info *upcall_info)
{
	struct ovs_header *upcall;
	struct sk_buff *nskb = NULL;
	struct sk_buff *user_skb; /* to be queued to userspace */
	struct nlattr *nla;
	struct genl_info info = {
		.dst_sk = ovs_dp_get_net(dp)->genl_sock,
		.snd_portid = upcall_info->portid,
	};
	size_t len;
	unsigned int hlen;
	int err, dp_ifindex;

	dp_ifindex = get_dpifindex(dp);
	if (!dp_ifindex)
		return -ENODEV;

	if (vlan_tx_tag_present(skb)) {
		nskb = skb_clone(skb, GFP_ATOMIC);
		if (!nskb)
			return -ENOMEM;

		nskb = __vlan_put_tag(nskb, nskb->vlan_proto, vlan_tx_tag_get(nskb));
		if (!nskb)
			return -ENOMEM;

		nskb->vlan_tci = 0;
		skb = nskb;
	}

	if (nla_attr_size(skb->len) > USHRT_MAX) {
		err = -EFBIG;
		goto out;
	}

	/* Complete checksum if needed */
	if (skb->ip_summed == CHECKSUM_PARTIAL &&
	    (err = skb_checksum_help(skb)))
		goto out;

	/* Older versions of OVS user space enforce alignment of the last
	 * Netlink attribute to NLA_ALIGNTO which would require extensive
	 * padding logic. Only perform zerocopy if padding is not required.
	 */
	if (dp->user_features & OVS_DP_F_UNALIGNED)
		hlen = skb_zerocopy_headlen(skb);
	else
		hlen = skb->len;

	len = upcall_msg_size(upcall_info->userdata, hlen);
	user_skb = genlmsg_new_unicast(len, &info, GFP_ATOMIC);
	if (!user_skb) {
		err = -ENOMEM;
		goto out;
	}

	upcall = genlmsg_put(user_skb, 0, 0, &dp_packet_genl_family,
			     0, upcall_info->cmd);
	upcall->dp_ifindex = dp_ifindex;

	nla = nla_nest_start(user_skb, OVS_PACKET_ATTR_KEY);
	ovs_nla_put_flow(upcall_info->key, upcall_info->key, user_skb);
	nla_nest_end(user_skb, nla);

	if (upcall_info->userdata)
		__nla_put(user_skb, OVS_PACKET_ATTR_USERDATA,
			  nla_len(upcall_info->userdata),
			  nla_data(upcall_info->userdata));

	/* Only reserve room for attribute header, packet data is added
	 * in skb_zerocopy() */
	if (!(nla = nla_reserve(user_skb, OVS_PACKET_ATTR_PACKET, 0))) {
		err = -ENOBUFS;
		goto out;
	}
	nla->nla_len = nla_attr_size(skb->len);

	skb_zerocopy(user_skb, skb, skb->len, hlen);

	/* Pad OVS_PACKET_ATTR_PACKET if linear copy was performed */
	if (!(dp->user_features & OVS_DP_F_UNALIGNED)) {
		size_t plen = NLA_ALIGN(user_skb->len) - user_skb->len;

		if (plen > 0)
			memset(skb_put(user_skb, plen), 0, plen);
	}

	((struct nlmsghdr *) user_skb->data)->nlmsg_len = user_skb->len;

	err = genlmsg_unicast(ovs_dp_get_net(dp), user_skb, upcall_info->portid);
out:
	kfree_skb(nskb);
	return err;
}