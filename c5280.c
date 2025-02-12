TPM_RC tpm_kdfa(TSS2_SYS_CONTEXT *sapi_context, TPMI_ALG_HASH hashAlg,
        TPM2B *key, char *label, TPM2B *contextU, TPM2B *contextV, UINT16 bits,
        TPM2B_MAX_BUFFER  *resultKey )
{
    TPM2B_DIGEST tmpResult;
    TPM2B_DIGEST tpm2bLabel, tpm2bBits, tpm2b_i_2;
    UINT8 *tpm2bBitsPtr = &tpm2bBits.t.buffer[0];
    UINT8 *tpm2b_i_2Ptr = &tpm2b_i_2.t.buffer[0];
    TPM2B_DIGEST *bufferList[8];
    UINT32 bitsSwizzled, i_Swizzled;
    TPM_RC rval;
    int i, j;
    UINT16 bytes = bits / 8;

    resultKey->t .size = 0;

    tpm2b_i_2.t.size = 4;

    tpm2bBits.t.size = 4;
    bitsSwizzled = string_bytes_endian_convert_32( bits );
    *(UINT32 *)tpm2bBitsPtr = bitsSwizzled;

    for(i = 0; label[i] != 0 ;i++ );

    tpm2bLabel.t.size = i+1;
    for( i = 0; i < tpm2bLabel.t.size; i++ )
    {
        tpm2bLabel.t.buffer[i] = label[i];
    }

    resultKey->t.size = 0;

    i = 1;

    while( resultKey->t.size < bytes )
    {
        // Inner loop

        i_Swizzled = string_bytes_endian_convert_32( i );
        *(UINT32 *)tpm2b_i_2Ptr = i_Swizzled;

        j = 0;
        bufferList[j++] = (TPM2B_DIGEST *)&(tpm2b_i_2.b);
        bufferList[j++] = (TPM2B_DIGEST *)&(tpm2bLabel.b);
        bufferList[j++] = (TPM2B_DIGEST *)contextU;
        bufferList[j++] = (TPM2B_DIGEST *)contextV;
        bufferList[j++] = (TPM2B_DIGEST *)&(tpm2bBits.b);
        bufferList[j++] = (TPM2B_DIGEST *)0;
        rval = tpm_hmac(sapi_context, hashAlg, key, (TPM2B **)&( bufferList[0] ), &tmpResult );
        if( rval != TPM_RC_SUCCESS )
        {
            return( rval );
        }

        bool res = string_bytes_concat_buffer(resultKey, &(tmpResult.b));
        if (!res) {
            return TSS2_SYS_RC_BAD_VALUE;
        }
    }

    // Truncate the result to the desired size.
    resultKey->t.size = bytes;

    return TPM_RC_SUCCESS;
}