TPMI_ALG_KDF_Unmarshal(TPMI_ALG_KDF *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_MGF1
	  case TPM_ALG_MGF1:
#endif
#if ALG_KDF1_SP800_56A
	  case TPM_ALG_KDF1_SP800_56A:	
#endif
#if ALG_KDF2
	  case TPM_ALG_KDF2:			
#endif
#if ALG_KDF1_SP800_108
	  case TPM_ALG_KDF1_SP800_108:	
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_KDF;
	}
    }
    return rc;
}