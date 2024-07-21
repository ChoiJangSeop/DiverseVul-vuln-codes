static BOOL ntlm_av_pair_add_copy(NTLM_AV_PAIR* pAvPairList, size_t cbAvPairList,
                                  NTLM_AV_PAIR* pAvPair, size_t cbAvPair)
{
	if (!ntlm_av_pair_check(pAvPair, cbAvPair))
		return FALSE;

	return ntlm_av_pair_add(pAvPairList, cbAvPairList, ntlm_av_pair_get_id(pAvPair),
	                        ntlm_av_pair_get_value_pointer(pAvPair), ntlm_av_pair_get_len(pAvPair));
}