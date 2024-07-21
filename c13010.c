TPMI_ALG_RSA_SCHEME_Unmarshal(TPMI_ALG_RSA_SCHEME *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_RSASSA
	  case TPM_ALG_RSASSA:
#endif
#if ALG_RSAPSS
	  case TPM_ALG_RSAPSS:
#endif
#if ALG_RSAES
	  case TPM_ALG_RSAES:
#endif
#if ALG_OAEP
	  case TPM_ALG_OAEP:
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}