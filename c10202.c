CryptSymmetricDecrypt(
		      BYTE                *dOut,          // OUT: decrypted data
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
    BYTE                *buffer;
    UINT32               buffersize = 0;
    BYTE                 keyToUse[MAX_SYM_KEY_BYTES];
    UINT16               keyToUseLen = (UINT16)sizeof(keyToUse);
    TPM_RC               retVal = TPM_RC_SUCCESS;

    // These are used but the compiler can't tell because they are initialized
    // in case statements and it can't tell if they are always initialized
    // when needed, so... Comment these out if the compiler can tell or doesn't
    // care that these are initialized before use.
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

    switch(mode)
	{
#if ALG_CBC || ALG_ECB
	  case ALG_CBC_VALUE:
	  case ALG_ECB_VALUE:
	    // For ECB and CBC, the data size must be an even multiple of the
	    // cipher block size
	    if((dSize % blockSize) != 0)
		return TPM_RC_SIZE;
	    break;
#endif
	  default:
	    break;
	}

    evpfn = GetEVPCipher(algorithm, keySizeInBits, mode, key,
                         keyToUse, &keyToUseLen);
    if (evpfn ==  NULL)
        return TPM_RC_FAILURE;

    /* a buffer with a 'safety margin' for EVP_DecryptUpdate */
    buffersize = TPM2_ROUNDUP(dSize + blockSize, blockSize);
    buffer = malloc(buffersize);
    if (buffer == NULL)
        ERROR_RETURN(TPM_RC_FAILURE);

#if ALG_TDES && ALG_CTR
    if (algorithm == TPM_ALG_TDES && mode == ALG_CTR_VALUE) {
        TDES_CTR(keyToUse, keyToUseLen * 8, dSize, dIn, iv, buffer, blockSize);
        outlen1 = dSize;
        ERROR_RETURN(TPM_RC_SUCCESS);
    }
#endif

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx ||
        EVP_DecryptInit_ex(ctx, evpfn(), NULL, keyToUse, iv) != 1 ||
        EVP_CIPHER_CTX_set_padding(ctx, 0) != 1 ||
        EVP_DecryptUpdate(ctx, buffer, &outlen1, dIn, dSize) != 1)
        ERROR_RETURN(TPM_RC_FAILURE);

    pAssert((int)buffersize >= outlen1);

    if ((int)buffersize <= outlen1 /* coverity */ ||
        EVP_DecryptFinal(ctx, &buffer[outlen1], &outlen2) != 1)
        ERROR_RETURN(TPM_RC_FAILURE);

    pAssert((int)buffersize >= outlen1 + outlen2);

 Exit:
    if (retVal == TPM_RC_SUCCESS) {
        pAssert(dSize >= outlen1 + outlen2);
        memcpy(dOut, buffer, outlen1 + outlen2);
    }

    clear_and_free(buffer, buffersize);
    EVP_CIPHER_CTX_free(ctx);

    return retVal;
}