static uint32_t get_lease(struct dhcp_packet *packet)
{
	uint8_t *option;
	uint32_t lease_seconds;

	option = dhcp_get_option(packet, DHCP_LEASE_TIME);
	if (!option)
		return 3600;

	lease_seconds = get_be32(option);

	if (lease_seconds < 10)
		lease_seconds = 10;

	return lease_seconds;
}