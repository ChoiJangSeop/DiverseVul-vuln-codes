TPMI_ALG_HASH_Unmarshal(TPMI_ALG_HASH *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_SHA1
	  case TPM_ALG_SHA1:
#endif
#if ALG_SHA256	
	  case TPM_ALG_SHA256:
#endif
#if ALG_SHA384
	  case 	TPM_ALG_SHA384:
#endif
#if ALG_SHA512
	  case 	TPM_ALG_SHA512:
#endif
#if ALG_SM3_256
	  case TPM_ALG_SM3_256:
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_HASH;
	}
    }
    return rc;
}