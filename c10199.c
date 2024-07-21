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
    BYTE                *pIv;
    int                  i;
    BYTE                 tmp[MAX_SYM_BLOCK_SIZE];
    BYTE                *pT;
    tpmCryptKeySchedule_t        keySchedule;
    INT16                blockSize;
    TpmCryptSetSymKeyCall_t        encrypt;
    BYTE                *iv;
    BYTE                 defaultIv[MAX_SYM_BLOCK_SIZE] = {0};
    //
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
    pIv = iv;
    // Create encrypt key schedule and set the encryption function pointer.
    SELECT(ENCRYPT);
    switch(mode)
	{
#if ALG_CTR
	  case ALG_CTR_VALUE:
	    for(; dSize > 0; dSize -= blockSize)
		{
		    // Encrypt the current value of the IV(counter)
		    ENCRYPT(&keySchedule, iv, tmp);
		    //increment the counter (counter is big-endian so start at end)
		    for(i = blockSize - 1; i >= 0; i--)
			if((iv[i] += 1) != 0)
			    break;
		    // XOR the encrypted counter value with input and put into output
		    pT = tmp;
		    for(i = (dSize < blockSize) ? dSize : blockSize; i > 0; i--)
			*dOut++ = *dIn++ ^ *pT++;
		}
	    break;
#endif
#if ALG_OFB
	  case ALG_OFB_VALUE:
	    // This is written so that dIn and dOut may be the same
	    for(; dSize > 0; dSize -= blockSize)
		{
		    // Encrypt the current value of the "IV"
		    ENCRYPT(&keySchedule, iv, iv);
		    // XOR the encrypted IV into dIn to create the cipher text (dOut)
		    pIv = iv;
		    for(i = (dSize < blockSize) ? dSize : blockSize; i > 0; i--)
			*dOut++ = (*pIv++ ^ *dIn++);
		}
	    break;
#endif
#if ALG_CBC
	  case ALG_CBC_VALUE:
	    // For CBC the data size must be an even multiple of the
	    // cipher block size
	    if((dSize % blockSize) != 0)
		return TPM_RC_SIZE;
	    // XOR the data block into the IV, encrypt the IV into the IV
	    // and then copy the IV to the output
	    for(; dSize > 0; dSize -= blockSize)
		{
		    pIv = iv;
		    for(i = blockSize; i > 0; i--)
			*pIv++ ^= *dIn++;
		    ENCRYPT(&keySchedule, iv, iv);
		    pIv = iv;
		    for(i = blockSize; i > 0; i--)
			*dOut++ = *pIv++;
		}
	    break;
#endif
	    // CFB is not optional
	  case ALG_CFB_VALUE:
	    // Encrypt the IV into the IV, XOR in the data, and copy to output
	    for(; dSize > 0; dSize -= blockSize)
		{
		    // Encrypt the current value of the IV
		    ENCRYPT(&keySchedule, iv, iv);
		    pIv = iv;
		    for(i = (int)(dSize < blockSize) ? dSize : blockSize; i > 0; i--)
			// XOR the data into the IV to create the cipher text
			// and put into the output
			*dOut++ = *pIv++ ^= *dIn++;
		}
	    // If the inner loop (i loop) was smaller than blockSize, then dSize
	    // would have been smaller than blockSize and it is now negative. If
	    // it is negative, then it indicates how many bytes are needed to pad
	    // out the IV for the next round.
	    for(; dSize < 0; dSize++)
		*pIv++ = 0;
	    break;
#if ALG_ECB
	  case ALG_ECB_VALUE:
	    // For ECB the data size must be an even multiple of the
	    // cipher block size
	    if((dSize % blockSize) != 0)
		return TPM_RC_SIZE;
	    // Encrypt the input block to the output block
	    for(; dSize > 0; dSize -= blockSize)
		{
		    ENCRYPT(&keySchedule, dIn, dOut);
		    dIn = &dIn[blockSize];
		    dOut = &dOut[blockSize];
		}
	    break;
#endif
	  default:
	    return TPM_RC_FAILURE;
	}
    return TPM_RC_SUCCESS;
}