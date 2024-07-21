TPM_SU_Unmarshal(TPM_SU *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_SU_CLEAR:
	  case TPM_SU_STATE:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}