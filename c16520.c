void bn_sqr_comba8(BN_ULONG *r, const BN_ULONG *a)
	{
	BN_ULONG t[16];
	bn_sqr_normal(r,a,8,t);
	}