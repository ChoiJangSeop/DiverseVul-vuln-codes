TPMI_ALG_SYM_MODE_Unmarshal(TPMI_ALG_SYM_MODE *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_CTR	
	  case TPM_ALG_CTR:
#endif
#if ALG_OFB	
	  case TPM_ALG_OFB:
#endif
#if ALG_CBC
	  case TPM_ALG_CBC:
#endif
#if ALG_CFB	
	  case TPM_ALG_CFB:
#endif
#if ALG_ECB	
	  case TPM_ALG_ECB:
#endif
#if ALG_CMAC
	  case TPM_ALG_CMAC:
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_MODE;
	}
    }
    return rc;
}