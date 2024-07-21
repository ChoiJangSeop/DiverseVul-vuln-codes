BnGeneratePrimeForRSA(
		      bigNum          prime,          // IN/OUT: points to the BN that will get the
		      //  random value
		      UINT32          bits,           // IN: number of bits to get
		      UINT32          exponent,       // IN: the exponent
		      RAND_STATE      *rand           // IN: the random state
		      )
{
    BOOL            found = FALSE;
    //
    // Make sure that the prime is large enough
    pAssert(prime->allocated >= BITS_TO_CRYPT_WORDS(bits));
    // Only try to handle specific sizes of keys in order to save overhead
    pAssert((bits % 32) == 0);
    
    prime->size = BITS_TO_CRYPT_WORDS(bits);
    
    while(!found)
	{
	    // The change below is to make sure that all keys that are generated from the same
	    // seed value will be the same regardless of the endianess or word size of the CPU.
	    //       DRBG_Generate(rand, (BYTE *)prime->d, (UINT16)BITS_TO_BYTES(bits));// old
	    //       if(g_inFailureMode)                                                // old
	// libtpms changed begin
	    switch (DRBG_GetSeedCompatLevel(rand)) {
	    case SEED_COMPAT_LEVEL_ORIGINAL:
		DRBG_Generate(rand, (BYTE *)prime->d, (UINT16)BITS_TO_BYTES(bits));
		if (g_inFailureMode)
		    return TPM_RC_FAILURE;
		break;
	    /* case SEED_COMPAT_LEVEL_LAST: */
            case SEED_COMPAT_LEVEL_RSA_PRIME_ADJUST_FIX:
		if(!BnGetRandomBits(prime, bits, rand))                              // new
		    return TPM_RC_FAILURE;
                break;
            default:
                FAIL(FATAL_ERROR_INTERNAL);
	    }
	    RsaAdjustPrimeCandidate(prime, DRBG_GetSeedCompatLevel(rand));
	// libtpms changed end
	    found = RsaCheckPrime(prime, exponent, rand) == TPM_RC_SUCCESS;
	}
    return TPM_RC_SUCCESS;
}