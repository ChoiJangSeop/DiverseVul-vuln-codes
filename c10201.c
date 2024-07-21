CryptSymmetricEncrypt(
		      BYTE                *dOut,          // OUT:
		      TPM_ALG_ID           algorithm,     // IN: the symmetric algorithm
		      UINT16               keySizeInBits, // IN: key size in bits
		      const BYTE          *key,           // IN: key buffer. The size of this buffer
		      //     in bytes is (keySizeInBits + 7) / 8
		      TPM2B_IV            *ivInOut,       // IN/OUT: IV for decryption.
		      TPM_ALG_ID           mode,          // IN: Mode to use
		      INT32                dSize,         // IN: data size (may need to be a
		      //     multiple of the blockSize)
		      const BYTE          *dIn            // IN: data buffer
		      )
{
    INT16                blockSize;
    BYTE                *iv;
    BYTE                 defaultIv[MAX_SYM_BLOCK_SIZE] = {0};
    evpfunc              evpfn;
    EVP_CIPHER_CTX      *ctx = NULL;
    int                  outlen1 = 0;
    int                  outlen2 = 0;
    BYTE                *pOut = dOut;
    BYTE                *buffer = NULL; // for in-place encryption
    UINT32               buffersize = 0;
    BYTE                 keyToUse[MAX_SYM_KEY_BYTES];
    UINT16               keyToUseLen = (UINT16)sizeof(keyToUse);
    TPM_RC               retVal = TPM_RC_SUCCESS;

    pAssert(dOut != NULL && key != NULL && dIn != NULL);
    if(dSize == 0)
	return TPM_RC_SUCCESS;
    TEST(algorithm);
    blockSize = CryptGetSymmetricBlockSize(algorithm, keySizeInBits);
    if(blockSize == 0)
	return TPM_RC_FAILURE;
    // If the iv is provided, then it is expected to be block sized. In some cases,
    // the caller is providing an array of 0's that is equal to [MAX_SYM_BLOCK_SIZE]
    // with no knowledge of the actual block size. This function will set it.
    if((ivInOut != NULL) && (mode != ALG_ECB_VALUE))
	{
	    ivInOut->t.size = blockSize;
	    iv = ivInOut->t.buffer;
	}
    else
	iv = defaultIv;

    switch (mode)
        {
          case ALG_ECB_VALUE:
          case ALG_CBC_VALUE:
	    // For ECB & CBC the data size must be an even multiple of the
	    // cipher block size
	    if((dSize % blockSize) != 0)
		return TPM_RC_SIZE;
        }

    evpfn = GetEVPCipher(algorithm, keySizeInBits, mode, key,
                         keyToUse, &keyToUseLen);
    if (evpfn == NULL)
        return TPM_RC_FAILURE;

    if (dIn == dOut) {
        // in-place encryption; we use a temp buffer
        buffersize = TPM2_ROUNDUP(dSize, blockSize);
        buffer = malloc(buffersize);
        if (buffer == NULL)
            ERROR_RETURN(TPM_RC_FAILURE);

        pOut = buffer;
    }

#if ALG_TDES && ALG_CTR
    if (algorithm == TPM_ALG_TDES && mode == ALG_CTR_VALUE) {
        TDES_CTR(keyToUse, keyToUseLen * 8, dSize, dIn, iv, pOut, blockSize);
        outlen1 = dSize;
        ERROR_RETURN(TPM_RC_SUCCESS);
    }
#endif

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx ||
        EVP_EncryptInit_ex(ctx, evpfn(), NULL, keyToUse, iv) != 1 ||
        EVP_CIPHER_CTX_set_padding(ctx, 0) != 1 ||
        EVP_EncryptUpdate(ctx, pOut, &outlen1, dIn, dSize) != 1)
        ERROR_RETURN(TPM_RC_FAILURE);

    pAssert(outlen1 <= dSize || dSize >= outlen1 + blockSize);

    if (EVP_EncryptFinal_ex(ctx, pOut + outlen1, &outlen2) != 1)
        ERROR_RETURN(TPM_RC_FAILURE);

 Exit:
    if (retVal == TPM_RC_SUCCESS && pOut != dOut)
        memcpy(dOut, pOut, outlen1 + outlen2);

    clear_and_free(buffer, buffersize);
    EVP_CIPHER_CTX_free(ctx);

    return retVal;
}