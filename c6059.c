int hns_rcb_get_ring_sset_count(int stringset)
{
	if (stringset == ETH_SS_STATS)
		return HNS_RING_STATIC_REG_NUM;

	return 0;
}