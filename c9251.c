void ntlm_print_av_pair_list(NTLM_AV_PAIR* pAvPairList, size_t cbAvPairList)
{
	size_t cbAvPair = cbAvPairList;
	NTLM_AV_PAIR* pAvPair = pAvPairList;

	if (!ntlm_av_pair_check(pAvPair, cbAvPair))
		return;

	WLog_INFO(TAG, "AV_PAIRs =");

	while (pAvPair && ntlm_av_pair_get_id(pAvPair) != MsvAvEOL)
	{
		WLog_INFO(TAG, "\t%s AvId: %" PRIu16 " AvLen: %" PRIu16 "",
		          AV_PAIR_STRINGS[ntlm_av_pair_get_id(pAvPair)], ntlm_av_pair_get_id(pAvPair),
		          ntlm_av_pair_get_len(pAvPair));
		winpr_HexDump(TAG, WLOG_INFO, ntlm_av_pair_get_value_pointer(pAvPair),
		              ntlm_av_pair_get_len(pAvPair));

		pAvPair = ntlm_av_pair_next(pAvPair, &cbAvPair);
	}
}