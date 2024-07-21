rx_cache_insert(netdissect_options *ndo,
                const u_char *bp, const struct ip *ip, u_int dport)
{
	struct rx_cache_entry *rxent;
	const struct rx_header *rxh = (const struct rx_header *) bp;

	if (!ND_TTEST_4(bp + sizeof(struct rx_header)))
		return;

	rxent = &rx_cache[rx_cache_next];

	if (++rx_cache_next >= RX_CACHE_SIZE)
		rx_cache_next = 0;

	rxent->callnum = EXTRACT_BE_U_4(rxh->callNumber);
	UNALIGNED_MEMCPY(&rxent->client, ip->ip_src, sizeof(uint32_t));
	UNALIGNED_MEMCPY(&rxent->server, ip->ip_dst, sizeof(uint32_t));
	rxent->dport = dport;
	rxent->serviceId = EXTRACT_BE_U_4(rxh->serviceId);
	rxent->opcode = EXTRACT_BE_U_4(bp + sizeof(struct rx_header));
}