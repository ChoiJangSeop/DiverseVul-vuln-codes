TPMI_ALG_SYM_OBJECT_Unmarshal(TPMI_ALG_SYM_OBJECT *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_AES
	  case TPM_ALG_AES:
#endif
#if ALG_SM4
	  case TPM_ALG_SM4:		
#endif
#if ALG_CAMELLIA
	  case TPM_ALG_CAMELLIA:	
#endif
#if ALG_TDES		// libtpms added begin
          case TPM_ALG_TDES:
#endif			// iibtpms added end
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_SYMMETRIC;
	}
    }
    return rc;
}