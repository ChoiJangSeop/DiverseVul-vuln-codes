static int l2tp_nl_cmd_session_create(struct sk_buff *skb, struct genl_info *info)
{
	u32 tunnel_id = 0;
	u32 session_id;
	u32 peer_session_id;
	int ret = 0;
	struct l2tp_tunnel *tunnel;
	struct l2tp_session *session;
	struct l2tp_session_cfg cfg = { 0, };
	struct net *net = genl_info_net(info);

	if (!info->attrs[L2TP_ATTR_CONN_ID]) {
		ret = -EINVAL;
		goto out;
	}

	tunnel_id = nla_get_u32(info->attrs[L2TP_ATTR_CONN_ID]);
	tunnel = l2tp_tunnel_get(net, tunnel_id);
	if (!tunnel) {
		ret = -ENODEV;
		goto out;
	}

	if (!info->attrs[L2TP_ATTR_SESSION_ID]) {
		ret = -EINVAL;
		goto out_tunnel;
	}
	session_id = nla_get_u32(info->attrs[L2TP_ATTR_SESSION_ID]);

	if (!info->attrs[L2TP_ATTR_PEER_SESSION_ID]) {
		ret = -EINVAL;
		goto out_tunnel;
	}
	peer_session_id = nla_get_u32(info->attrs[L2TP_ATTR_PEER_SESSION_ID]);

	if (!info->attrs[L2TP_ATTR_PW_TYPE]) {
		ret = -EINVAL;
		goto out_tunnel;
	}
	cfg.pw_type = nla_get_u16(info->attrs[L2TP_ATTR_PW_TYPE]);
	if (cfg.pw_type >= __L2TP_PWTYPE_MAX) {
		ret = -EINVAL;
		goto out_tunnel;
	}

	if (tunnel->version > 2) {
		if (info->attrs[L2TP_ATTR_OFFSET])
			cfg.offset = nla_get_u16(info->attrs[L2TP_ATTR_OFFSET]);

		if (info->attrs[L2TP_ATTR_DATA_SEQ])
			cfg.data_seq = nla_get_u8(info->attrs[L2TP_ATTR_DATA_SEQ]);

		cfg.l2specific_type = L2TP_L2SPECTYPE_DEFAULT;
		if (info->attrs[L2TP_ATTR_L2SPEC_TYPE])
			cfg.l2specific_type = nla_get_u8(info->attrs[L2TP_ATTR_L2SPEC_TYPE]);

		cfg.l2specific_len = 4;
		if (info->attrs[L2TP_ATTR_L2SPEC_LEN])
			cfg.l2specific_len = nla_get_u8(info->attrs[L2TP_ATTR_L2SPEC_LEN]);

		if (info->attrs[L2TP_ATTR_COOKIE]) {
			u16 len = nla_len(info->attrs[L2TP_ATTR_COOKIE]);
			if (len > 8) {
				ret = -EINVAL;
				goto out_tunnel;
			}
			cfg.cookie_len = len;
			memcpy(&cfg.cookie[0], nla_data(info->attrs[L2TP_ATTR_COOKIE]), len);
		}
		if (info->attrs[L2TP_ATTR_PEER_COOKIE]) {
			u16 len = nla_len(info->attrs[L2TP_ATTR_PEER_COOKIE]);
			if (len > 8) {
				ret = -EINVAL;
				goto out_tunnel;
			}
			cfg.peer_cookie_len = len;
			memcpy(&cfg.peer_cookie[0], nla_data(info->attrs[L2TP_ATTR_PEER_COOKIE]), len);
		}
		if (info->attrs[L2TP_ATTR_IFNAME])
			cfg.ifname = nla_data(info->attrs[L2TP_ATTR_IFNAME]);

		if (info->attrs[L2TP_ATTR_VLAN_ID])
			cfg.vlan_id = nla_get_u16(info->attrs[L2TP_ATTR_VLAN_ID]);
	}

	if (info->attrs[L2TP_ATTR_DEBUG])
		cfg.debug = nla_get_u32(info->attrs[L2TP_ATTR_DEBUG]);

	if (info->attrs[L2TP_ATTR_RECV_SEQ])
		cfg.recv_seq = nla_get_u8(info->attrs[L2TP_ATTR_RECV_SEQ]);

	if (info->attrs[L2TP_ATTR_SEND_SEQ])
		cfg.send_seq = nla_get_u8(info->attrs[L2TP_ATTR_SEND_SEQ]);

	if (info->attrs[L2TP_ATTR_LNS_MODE])
		cfg.lns_mode = nla_get_u8(info->attrs[L2TP_ATTR_LNS_MODE]);

	if (info->attrs[L2TP_ATTR_RECV_TIMEOUT])
		cfg.reorder_timeout = nla_get_msecs(info->attrs[L2TP_ATTR_RECV_TIMEOUT]);

	if (info->attrs[L2TP_ATTR_MTU])
		cfg.mtu = nla_get_u16(info->attrs[L2TP_ATTR_MTU]);

	if (info->attrs[L2TP_ATTR_MRU])
		cfg.mru = nla_get_u16(info->attrs[L2TP_ATTR_MRU]);

#ifdef CONFIG_MODULES
	if (l2tp_nl_cmd_ops[cfg.pw_type] == NULL) {
		genl_unlock();
		request_module("net-l2tp-type-%u", cfg.pw_type);
		genl_lock();
	}
#endif
	if ((l2tp_nl_cmd_ops[cfg.pw_type] == NULL) ||
	    (l2tp_nl_cmd_ops[cfg.pw_type]->session_create == NULL)) {
		ret = -EPROTONOSUPPORT;
		goto out_tunnel;
	}

	/* Check that pseudowire-specific params are present */
	switch (cfg.pw_type) {
	case L2TP_PWTYPE_NONE:
		break;
	case L2TP_PWTYPE_ETH_VLAN:
		if (!info->attrs[L2TP_ATTR_VLAN_ID]) {
			ret = -EINVAL;
			goto out_tunnel;
		}
		break;
	case L2TP_PWTYPE_ETH:
		break;
	case L2TP_PWTYPE_PPP:
	case L2TP_PWTYPE_PPP_AC:
		break;
	case L2TP_PWTYPE_IP:
	default:
		ret = -EPROTONOSUPPORT;
		break;
	}

	ret = -EPROTONOSUPPORT;
	if (l2tp_nl_cmd_ops[cfg.pw_type]->session_create)
		ret = (*l2tp_nl_cmd_ops[cfg.pw_type]->session_create)(net, tunnel_id,
			session_id, peer_session_id, &cfg);

	if (ret >= 0) {
		session = l2tp_session_get(net, tunnel, session_id, false);
		if (session) {
			ret = l2tp_session_notify(&l2tp_nl_family, info, session,
						  L2TP_CMD_SESSION_CREATE);
			l2tp_session_dec_refcount(session);
		}
	}

out_tunnel:
	l2tp_tunnel_dec_refcount(tunnel);
out:
	return ret;
}