TPM_SE_Unmarshal(TPM_SE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT8_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_SE_HMAC:
	  case TPM_SE_POLICY:
	  case TPM_SE_TRIAL:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}