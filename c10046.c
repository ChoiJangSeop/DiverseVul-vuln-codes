static gboolean listener_event(GIOChannel *channel, GIOCondition condition,
							gpointer user_data)
{
	GDHCPServer *dhcp_server = user_data;
	struct dhcp_packet packet;
	struct dhcp_lease *lease;
	uint32_t requested_nip = 0;
	uint8_t type, *server_id_option, *request_ip_option;
	int re;

	if (condition & (G_IO_NVAL | G_IO_ERR | G_IO_HUP)) {
		dhcp_server->listener_watch = 0;
		return FALSE;
	}

	re = dhcp_recv_l3_packet(&packet, dhcp_server->listener_sockfd);
	if (re < 0)
		return TRUE;

	type = check_packet_type(&packet);
	if (type == 0)
		return TRUE;

	server_id_option = dhcp_get_option(&packet, DHCP_SERVER_ID);
	if (server_id_option) {
		uint32_t server_nid =
			get_unaligned((const uint32_t *) server_id_option);

		if (server_nid != dhcp_server->server_nip)
			return TRUE;
	}

	request_ip_option = dhcp_get_option(&packet, DHCP_REQUESTED_IP);
	if (request_ip_option)
		requested_nip = get_be32(request_ip_option);

	lease = find_lease_by_mac(dhcp_server, packet.chaddr);

	switch (type) {
	case DHCPDISCOVER:
		debug(dhcp_server, "Received DISCOVER");

		send_offer(dhcp_server, &packet, lease, requested_nip);
		break;
	case DHCPREQUEST:
		debug(dhcp_server, "Received REQUEST NIP %d",
							requested_nip);
		if (requested_nip == 0) {
			requested_nip = ntohl(packet.ciaddr);
			if (requested_nip == 0)
				break;
		}

		if (lease && requested_nip == lease->lease_nip) {
			debug(dhcp_server, "Sending ACK");
			send_ACK(dhcp_server, &packet,
				lease->lease_nip);
			break;
		}

		if (server_id_option || !lease) {
			debug(dhcp_server, "Sending NAK");
			send_NAK(dhcp_server, &packet);
		}

		break;
	case DHCPDECLINE:
		debug(dhcp_server, "Received DECLINE");

		if (!server_id_option)
			break;

		if (!request_ip_option)
			break;

		if (!lease)
			break;

		if (requested_nip == lease->lease_nip)
			remove_lease(dhcp_server, lease);

		break;
	case DHCPRELEASE:
		debug(dhcp_server, "Received RELEASE");

		if (!server_id_option)
			break;

		if (!lease)
			break;

		if (packet.ciaddr == lease->lease_nip)
			lease_set_expire(dhcp_server, lease,
					time(NULL));
		break;
	case DHCPINFORM:
		debug(dhcp_server, "Received INFORM");
		send_inform(dhcp_server, &packet);
		break;
	}

	return TRUE;
}