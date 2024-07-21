void bn_mul_comba8(BN_ULONG *r, BN_ULONG *a, BN_ULONG *b)
	{
	r[ 8]=bn_mul_words(    &(r[0]),a,8,b[0]);
	r[ 9]=bn_mul_add_words(&(r[1]),a,8,b[1]);
	r[10]=bn_mul_add_words(&(r[2]),a,8,b[2]);
	r[11]=bn_mul_add_words(&(r[3]),a,8,b[3]);
	r[12]=bn_mul_add_words(&(r[4]),a,8,b[4]);
	r[13]=bn_mul_add_words(&(r[5]),a,8,b[5]);
	r[14]=bn_mul_add_words(&(r[6]),a,8,b[6]);
	r[15]=bn_mul_add_words(&(r[7]),a,8,b[7]);
	}