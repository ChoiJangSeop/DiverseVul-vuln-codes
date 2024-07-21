static uint8_t check_packet_type(struct dhcp_packet *packet)
{
	uint8_t *type;

	if (packet->hlen != ETH_ALEN)
		return 0;

	if (packet->op != BOOTREQUEST)
		return 0;

	type = dhcp_get_option(packet, DHCP_MESSAGE_TYPE);

	if (!type)
		return 0;

	if (*type < DHCP_MINTYPE)
		return 0;

	if (*type > DHCP_MAXTYPE)
		return 0;

	return *type;
}