static int jpc_calcssmant(jpc_fix_t stepsize)
{
	int n;
	int e;
	int m;

	n = jpc_firstone(stepsize);
	e = n - JPC_FIX_FRACBITS;
	if (n >= 11) {
		m = (stepsize >> (n - 11)) & 0x7ff;
	} else {
		m = (stepsize & ((1 << n) - 1)) << (11 - n);
	}
	return m;
}