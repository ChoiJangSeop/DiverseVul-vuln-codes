TPMI_ST_ATTEST_Unmarshal(TPMI_ST_ATTEST *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ST_Unmarshal(target, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_ST_ATTEST_CERTIFY:
	  case TPM_ST_ATTEST_CREATION:
	  case TPM_ST_ATTEST_QUOTE:
	  case TPM_ST_ATTEST_COMMAND_AUDIT:
	  case TPM_ST_ATTEST_SESSION_AUDIT:
	  case TPM_ST_ATTEST_TIME:
	  case TPM_ST_ATTEST_NV:
	    break;
	  default:
	    rc = TPM_RC_SELECTOR;
	}
    }
    return rc;
}