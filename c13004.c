TPM_ST_Unmarshal(TPM_ST *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_ST_RSP_COMMAND:
	  case TPM_ST_NULL:
	  case TPM_ST_NO_SESSIONS:
	  case TPM_ST_SESSIONS:
	  case TPM_ST_ATTEST_NV:
	  case TPM_ST_ATTEST_COMMAND_AUDIT:
	  case TPM_ST_ATTEST_SESSION_AUDIT:
	  case TPM_ST_ATTEST_CERTIFY:
	  case TPM_ST_ATTEST_QUOTE:
	  case TPM_ST_ATTEST_TIME:
	  case TPM_ST_ATTEST_CREATION:
	  case TPM_ST_CREATION:
	  case TPM_ST_VERIFIED:
	  case TPM_ST_AUTH_SECRET:
	  case TPM_ST_HASHCHECK:
	  case TPM_ST_AUTH_SIGNED:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}