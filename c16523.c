void bn_mul_comba4(BN_ULONG *r, BN_ULONG *a, BN_ULONG *b)
	{
	r[4]=bn_mul_words(    &(r[0]),a,4,b[0]);
	r[5]=bn_mul_add_words(&(r[1]),a,4,b[1]);
	r[6]=bn_mul_add_words(&(r[2]),a,4,b[2]);
	r[7]=bn_mul_add_words(&(r[3]),a,4,b[3]);
	}