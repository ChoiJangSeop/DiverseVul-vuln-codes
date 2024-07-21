void bn_sqr_comba4(BN_ULONG *r, const BN_ULONG *a)
	{
	BN_ULONG t[8];
	bn_sqr_normal(r,a,4,t);
	}