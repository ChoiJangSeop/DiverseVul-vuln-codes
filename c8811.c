rx_cache_find(const struct rx_header *rxh, const struct ip *ip, u_int sport,
	      uint32_t *opcode)
{
	uint32_t i;
	struct rx_cache_entry *rxent;
	uint32_t clip;
	uint32_t sip;

	UNALIGNED_MEMCPY(&clip, ip->ip_dst, sizeof(uint32_t));
	UNALIGNED_MEMCPY(&sip, ip->ip_src, sizeof(uint32_t));

	/* Start the search where we last left off */

	i = rx_cache_hint;
	do {
		rxent = &rx_cache[i];
		if (rxent->callnum == EXTRACT_BE_U_4(rxh->callNumber) &&
		    rxent->client.s_addr == clip &&
		    rxent->server.s_addr == sip &&
		    rxent->serviceId == EXTRACT_BE_U_4(rxh->serviceId) &&
		    rxent->dport == sport) {

			/* We got a match! */

			rx_cache_hint = i;
			*opcode = rxent->opcode;
			return(1);
		}
		if (++i >= RX_CACHE_SIZE)
			i = 0;
	} while (i != rx_cache_hint);

	/* Our search failed */
	return(0);
}