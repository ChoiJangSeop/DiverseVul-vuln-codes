static size_t ntlm_av_pair_get_next_offset(NTLM_AV_PAIR* pAvPair)
{
	return ntlm_av_pair_get_len(pAvPair) + sizeof(NTLM_AV_PAIR);
}