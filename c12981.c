TPMI_ALG_PUBLIC_Unmarshal(TPMI_ALG_PUBLIC *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_KEYEDHASH
	  case TPM_ALG_KEYEDHASH:
#endif
#if ALG_RSA
	  case TPM_ALG_RSA:
#endif
#if ALG_ECC
	  case TPM_ALG_ECC:
#endif
#if ALG_SYMCIPHER
	  case TPM_ALG_SYMCIPHER:
#endif
	    break;
	  default:
	    rc = TPM_RC_TYPE;
	}
    }
    return rc;
}