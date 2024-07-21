BnGeneratePrimeForRSA(
		      bigNum          prime,
		      UINT32          bits,
		      UINT32          exponent,
		      RAND_STATE      *rand
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
	    DRBG_Generate(rand, (BYTE *)prime->d, (UINT16)BITS_TO_BYTES(bits));
	    if(g_inFailureMode)
		return TPM_RC_FAILURE;
	    RsaAdjustPrimeCandidate(prime);
	    found = RsaCheckPrime(prime, exponent, rand) == TPM_RC_SUCCESS;
	}
    return TPM_RC_SUCCESS;
}