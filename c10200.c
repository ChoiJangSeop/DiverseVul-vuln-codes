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
    BYTE                *pIv;
    int                  i;
    BYTE                 tmp[MAX_SYM_BLOCK_SIZE];
    BYTE                *pT;
    tpmCryptKeySchedule_t        keySchedule;
    INT16                blockSize;
    BYTE                *iv;
    TpmCryptSetSymKeyCall_t        encrypt;
    TpmCryptSetSymKeyCall_t        decrypt;
    BYTE                 defaultIv[MAX_SYM_BLOCK_SIZE] = {0};
    // These are used but the compiler can't tell because they are initialized
    // in case statements and it can't tell if they are always initialized
    // when needed, so... Comment these out if the compiler can tell or doesn't
    // care that these are initialized before use.
    encrypt = NULL;
    decrypt = NULL;
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
    // Use the mode to select the key schedule to create. Encrypt always uses the
    // encryption schedule. Depending on the mode, decryption might use either
    // the decryption or encryption schedule.
    switch(mode)
	{
#if ALG_CBC || ALG_ECB
	  case ALG_CBC_VALUE: // decrypt = decrypt
	  case ALG_ECB_VALUE:
	    // For ECB and CBC, the data size must be an even multiple of the
	    // cipher block size
	    if((dSize % blockSize) != 0)
		return TPM_RC_SIZE;
	    SELECT(DECRYPT);
	    break;
#endif
	  default:
	    // For the remaining stream ciphers, use encryption to decrypt
	    SELECT(ENCRYPT);
	    break;
	}
    // Now do the mode-dependent decryption
    switch(mode)
	{
#if ALG_CBC
	  case ALG_CBC_VALUE:
	    // Copy the input data to a temp buffer, decrypt the buffer into the
	    // output, XOR in the IV, and copy the temp buffer to the IV and repeat.
	    for(; dSize > 0; dSize -= blockSize)
		{
		    pT = tmp;
		    for(i = blockSize; i > 0; i--)
			*pT++ = *dIn++;
		    DECRYPT(&keySchedule, tmp, dOut);
		    pIv = iv;
		    pT = tmp;
		    for(i = blockSize; i > 0; i--)
			{
			    *dOut++ ^= *pIv;
			    *pIv++ = *pT++;
			}
		}
	    break;
#endif
	  case TPM_ALG_CFB:
	    for(; dSize > 0; dSize -= blockSize)
		{
		    // Encrypt the IV into the temp buffer
		    ENCRYPT(&keySchedule, iv, tmp);
		    pT = tmp;
		    pIv = iv;
		    for(i = (dSize < blockSize) ? dSize : blockSize; i > 0; i--)
			// Copy the current cipher text to IV, XOR
			// with the temp buffer and put into the output
			*dOut++ = *pT++ ^ (*pIv++ = *dIn++);
		}
	    // If the inner loop (i loop) was smaller than blockSize, then dSize
	    // would have been smaller than blockSize and it is now negative
	    // If it is negative, then it indicates how may fill bytes
	    // are needed to pad out the IV for the next round.
	    for(; dSize < 0; dSize++)
		*pIv++ = 0;
	    break;
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
#if ALG_ECB
	  case ALG_ECB_VALUE:
	    for(; dSize > 0; dSize -= blockSize)
		{
		    DECRYPT(&keySchedule, dIn, dOut);
		    dIn = &dIn[blockSize];
		    dOut = &dOut[blockSize];
		}
	    break;
#endif
#if ALG_OFB
	  case TPM_ALG_OFB:
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
	  default:
	    return TPM_RC_FAILURE;
	}
    return TPM_RC_SUCCESS;
}