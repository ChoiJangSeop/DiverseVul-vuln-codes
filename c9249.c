static BOOL ntlm_av_pair_check(NTLM_AV_PAIR* pAvPair, size_t cbAvPair)
{
	if (!pAvPair || cbAvPair < sizeof(NTLM_AV_PAIR))
		return FALSE;
	return cbAvPair >= ntlm_av_pair_get_next_offset(pAvPair);
}