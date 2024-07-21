static gboolean listener_event(GIOChannel *channel, GIOCondition condition,
							gpointer user_data)
{
	GDHCPClient *dhcp_client = user_data;
	struct sockaddr_in dst_addr = { 0 };
	struct dhcp_packet packet;
	struct dhcpv6_packet *packet6 = NULL;
	uint8_t *message_type = NULL, *client_id = NULL, *option,
		*server_id = NULL;
	uint16_t option_len = 0, status = 0;
	uint32_t xid = 0;
	gpointer pkt;
	unsigned char buf[MAX_DHCPV6_PKT_SIZE];
	uint16_t pkt_len = 0;
	int count;
	int re;

	if (condition & (G_IO_NVAL | G_IO_ERR | G_IO_HUP)) {
		dhcp_client->listener_watch = 0;
		return FALSE;
	}

	if (dhcp_client->listen_mode == L_NONE)
		return FALSE;

	pkt = &packet;

	dhcp_client->status_code = 0;

	if (dhcp_client->listen_mode == L2) {
		re = dhcp_recv_l2_packet(&packet,
					dhcp_client->listener_sockfd,
					&dst_addr);
		pkt_len = (uint16_t)(unsigned int)re;
		xid = packet.xid;
	} else if (dhcp_client->listen_mode == L3) {
		if (dhcp_client->type == G_DHCP_IPV6) {
			re = dhcpv6_recv_l3_packet(&packet6, buf, sizeof(buf),
						dhcp_client->listener_sockfd);
			if (re < 0)
			    return TRUE;
			pkt_len = re;
			pkt = packet6;
			xid = packet6->transaction_id[0] << 16 |
				packet6->transaction_id[1] << 8 |
				packet6->transaction_id[2];
		} else {
			re = dhcp_recv_l3_packet(&packet,
						dhcp_client->listener_sockfd);
			xid = packet.xid;
		}
	} else if (dhcp_client->listen_mode == L_ARP) {
		ipv4ll_recv_arp_packet(dhcp_client);
		return TRUE;
	} else
		re = -EIO;

	if (re < 0)
		return TRUE;

	if (!check_package_owner(dhcp_client, pkt))
		return TRUE;

	if (dhcp_client->type == G_DHCP_IPV6) {
		if (!packet6)
			return TRUE;

		count = 0;
		client_id = dhcpv6_get_option(packet6, pkt_len,
				G_DHCPV6_CLIENTID, &option_len,	&count);

		if (!client_id || count == 0 || option_len == 0 ||
				memcmp(dhcp_client->duid, client_id,
					dhcp_client->duid_len) != 0) {
			debug(dhcp_client,
				"client duid error, discarding msg %p/%d/%d",
				client_id, option_len, count);
			return TRUE;
		}

		option = dhcpv6_get_option(packet6, pkt_len,
				G_DHCPV6_STATUS_CODE, &option_len, NULL);
		if (option != 0 && option_len > 0) {
			status = option[0]<<8 | option[1];
			if (status != 0) {
				debug(dhcp_client, "error code %d", status);
				if (option_len > 2) {
					gchar *txt = g_strndup(
						(gchar *)&option[2],
						option_len - 2);
					debug(dhcp_client, "error text: %s",
						txt);
					g_free(txt);
				}
			}
			dhcp_client->status_code = status;
		}
	} else {
		message_type = dhcp_get_option(&packet, pkt_len, DHCP_MESSAGE_TYPE);
		if (!message_type)
			return TRUE;
	}

	debug(dhcp_client, "received DHCP packet xid 0x%04x "
		"(current state %d)", ntohl(xid), dhcp_client->state);

	switch (dhcp_client->state) {
	case INIT_SELECTING:
		if (*message_type != DHCPOFFER)
			return TRUE;

		remove_timeouts(dhcp_client);
		dhcp_client->timeout = 0;
		dhcp_client->retry_times = 0;

		option = dhcp_get_option(&packet, pkt_len, DHCP_SERVER_ID);
		dhcp_client->server_ip = get_be32(option);
		dhcp_client->requested_ip = ntohl(packet.yiaddr);

		dhcp_client->state = REQUESTING;

		/*
		 * RFC2131:
		 *
		 *   If unicasting is not possible, the message MAY be
		 *   sent as an IP broadcast using an IP broadcast address
		 *   (preferably 0xffffffff) as the IP destination address
		 *   and the link-layer broadcast address as the link-layer
		 *   destination address.
		 *
		 * For interoperability reasons, if the response is an IP
		 * broadcast, let's reuse broadcast flag from DHCPDISCOVER
		 * to which the server has responded. Some servers are picky
		 * about this flag.
		 */
		dhcp_client->request_bcast =
			dst_addr.sin_addr.s_addr == INADDR_BROADCAST &&
			g_hash_table_contains(dhcp_client->secs_bcast_hash,
				GINT_TO_POINTER(packet.secs));

		debug(dhcp_client, "init ip %s secs %hu -> broadcast flag %s",
			inet_ntoa(dst_addr.sin_addr), packet.secs,
			dhcp_client->request_bcast ? "on" : "off");

		start_request(dhcp_client);

		return TRUE;
	case REBOOTING:
		if (dst_addr.sin_addr.s_addr == INADDR_BROADCAST)
			dhcp_client->request_bcast = true;
		else
			dhcp_client->request_bcast = false;

		debug(dhcp_client, "ip %s -> %sadding broadcast flag",
			inet_ntoa(dst_addr.sin_addr),
			dhcp_client->request_bcast ? "" : "not ");
		/* fall through */
	case REQUESTING:
	case RENEWING:
	case REBINDING:
		if (*message_type == DHCPACK) {
			dhcp_client->retry_times = 0;

			remove_timeouts(dhcp_client);

			dhcp_client->lease_seconds = get_lease(&packet, pkt_len);

			get_request(dhcp_client, &packet, pkt_len);

			switch_listening_mode(dhcp_client, L_NONE);

			g_free(dhcp_client->assigned_ip);
			dhcp_client->assigned_ip = get_ip(packet.yiaddr);

			if (dhcp_client->state == REBOOTING) {
				option = dhcp_get_option(&packet, pkt_len,
							DHCP_SERVER_ID);
				dhcp_client->server_ip = get_be32(option);
			}

			/* Address should be set up here */
			if (dhcp_client->lease_available_cb)
				dhcp_client->lease_available_cb(dhcp_client,
					dhcp_client->lease_available_data);

			start_bound(dhcp_client);
		} else if (*message_type == DHCPNAK) {
			dhcp_client->retry_times = 0;

			remove_timeouts(dhcp_client);

			dhcp_client->timeout = g_timeout_add_seconds_full(
							G_PRIORITY_HIGH, 3,
							restart_dhcp_timeout,
							dhcp_client,
							NULL);
		}

		break;
	case SOLICITATION:
		if (dhcp_client->type != G_DHCP_IPV6)
			return TRUE;

		if (packet6->message != DHCPV6_REPLY &&
				packet6->message != DHCPV6_ADVERTISE)
			return TRUE;

		count = 0;
		server_id = dhcpv6_get_option(packet6, pkt_len,
				G_DHCPV6_SERVERID, &option_len,	&count);
		if (!server_id || count != 1 || option_len == 0) {
			/* RFC 3315, 15.10 */
			debug(dhcp_client,
				"server duid error, discarding msg %p/%d/%d",
				server_id, option_len, count);
			return TRUE;
		}
		dhcp_client->server_duid = g_try_malloc(option_len);
		if (!dhcp_client->server_duid)
			return TRUE;
		memcpy(dhcp_client->server_duid, server_id, option_len);
		dhcp_client->server_duid_len = option_len;

		if (packet6->message == DHCPV6_REPLY) {
			uint8_t *rapid_commit;
			count = 0;
			option_len = 0;
			rapid_commit = dhcpv6_get_option(packet6, pkt_len,
							G_DHCPV6_RAPID_COMMIT,
							&option_len, &count);
			if (!rapid_commit || option_len != 0 ||
								count != 1)
				/* RFC 3315, 17.1.4 */
				return TRUE;
		}

		switch_listening_mode(dhcp_client, L_NONE);

		if (dhcp_client->status_code == 0)
			get_dhcpv6_request(dhcp_client, packet6, pkt_len,
					&dhcp_client->status_code);

		if (packet6->message == DHCPV6_ADVERTISE) {
			if (dhcp_client->advertise_cb)
				dhcp_client->advertise_cb(dhcp_client,
						dhcp_client->advertise_data);
			return TRUE;
		}

		if (dhcp_client->solicitation_cb) {
			/*
			 * The dhcp_client might not be valid after the
			 * callback call so just return immediately.
			 */
			dhcp_client->solicitation_cb(dhcp_client,
					dhcp_client->solicitation_data);
			return TRUE;
		}
		break;
	case REBIND:
		if (dhcp_client->type != G_DHCP_IPV6)
			return TRUE;

		server_id = dhcpv6_get_option(packet6, pkt_len,
				G_DHCPV6_SERVERID, &option_len,	&count);
		if (!dhcp_client->server_duid && server_id &&
								count == 1) {
			/*
			 * If we do not have server duid yet, then get it now.
			 * Prefix delegation renew support needs it.
			 */
			dhcp_client->server_duid = g_try_malloc(option_len);
			if (!dhcp_client->server_duid)
				return TRUE;
			memcpy(dhcp_client->server_duid, server_id, option_len);
			dhcp_client->server_duid_len = option_len;
		}
		/* fall through */
	case INFORMATION_REQ:
	case REQUEST:
	case RENEW:
	case RELEASE:
	case CONFIRM:
	case DECLINE:
		if (dhcp_client->type != G_DHCP_IPV6)
			return TRUE;

		if (packet6->message != DHCPV6_REPLY)
			return TRUE;

		count = 0;
		option_len = 0;
		server_id = dhcpv6_get_option(packet6, pkt_len,
				G_DHCPV6_SERVERID, &option_len, &count);
		if (!server_id || count != 1 || option_len == 0 ||
				(dhcp_client->server_duid_len > 0 &&
				memcmp(dhcp_client->server_duid, server_id,
					dhcp_client->server_duid_len) != 0)) {
			/* RFC 3315, 15.10 */
			debug(dhcp_client,
				"server duid error, discarding msg %p/%d/%d",
				server_id, option_len, count);
			return TRUE;
		}

		switch_listening_mode(dhcp_client, L_NONE);

		get_dhcpv6_request(dhcp_client, packet6, pkt_len,
						&dhcp_client->status_code);

		if (dhcp_client->information_req_cb) {
			/*
			 * The dhcp_client might not be valid after the
			 * callback call so just return immediately.
			 */
			dhcp_client->information_req_cb(dhcp_client,
					dhcp_client->information_req_data);
			return TRUE;
		}
		if (dhcp_client->request_cb) {
			dhcp_client->request_cb(dhcp_client,
					dhcp_client->request_data);
			return TRUE;
		}
		if (dhcp_client->renew_cb) {
			dhcp_client->renew_cb(dhcp_client,
					dhcp_client->renew_data);
			return TRUE;
		}
		if (dhcp_client->rebind_cb) {
			dhcp_client->rebind_cb(dhcp_client,
					dhcp_client->rebind_data);
			return TRUE;
		}
		if (dhcp_client->release_cb) {
			dhcp_client->release_cb(dhcp_client,
					dhcp_client->release_data);
			return TRUE;
		}
		if (dhcp_client->decline_cb) {
			dhcp_client->decline_cb(dhcp_client,
					dhcp_client->decline_data);
			return TRUE;
		}
		if (dhcp_client->confirm_cb) {
			count = 0;
			server_id = dhcpv6_get_option(packet6, pkt_len,
						G_DHCPV6_SERVERID, &option_len,
						&count);
			if (!server_id || count != 1 ||
							option_len == 0) {
				/* RFC 3315, 15.10 */
				debug(dhcp_client,
					"confirm server duid error, "
					"discarding msg %p/%d/%d",
					server_id, option_len, count);
				return TRUE;
			}
			dhcp_client->server_duid = g_try_malloc(option_len);
			if (!dhcp_client->server_duid)
				return TRUE;
			memcpy(dhcp_client->server_duid, server_id, option_len);
			dhcp_client->server_duid_len = option_len;

			dhcp_client->confirm_cb(dhcp_client,
						dhcp_client->confirm_data);
			return TRUE;
		}
		break;
	default:
		break;
	}

	debug(dhcp_client, "processed DHCP packet (new state %d)",
							dhcp_client->state);

	return TRUE;
}