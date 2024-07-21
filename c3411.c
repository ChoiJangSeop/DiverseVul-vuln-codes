mem_stats(
	sockaddr_u *srcadr,
	endpt *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_mem_stats *ms;
	register int i;

	ms = (struct info_mem_stats *)prepare_pkt(srcadr, inter, inpkt,
						  sizeof(struct info_mem_stats));

	ms->timereset = htonl((u_int32)(current_time - peer_timereset));
	ms->totalpeermem = htons((u_short)total_peer_structs);
	ms->freepeermem = htons((u_short)peer_free_count);
	ms->findpeer_calls = htonl((u_int32)findpeer_calls);
	ms->allocations = htonl((u_int32)peer_allocations);
	ms->demobilizations = htonl((u_int32)peer_demobilizations);

	for (i = 0; i < NTP_HASH_SIZE; i++)
		ms->hashcount[i] = (u_char)
		    max((u_int)peer_hash_count[i], UCHAR_MAX);

	more_pkt();
	flush_pkt();
}