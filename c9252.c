static NTLM_AV_PAIR* ntlm_av_pair_next(NTLM_AV_PAIR* pAvPair, size_t* pcbAvPair)
{
	size_t offset;

	if (!pcbAvPair)
		return NULL;
	if (!ntlm_av_pair_check(pAvPair, *pcbAvPair))
		return NULL;

	offset = ntlm_av_pair_get_next_offset(pAvPair);
	*pcbAvPair -= offset;
	return (NTLM_AV_PAIR*)((PBYTE)pAvPair + offset);
}