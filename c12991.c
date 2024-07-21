TPMI_ST_COMMAND_TAG_Unmarshal(TPMI_ST_COMMAND_TAG *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ST_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_ST_NO_SESSIONS:
	  case TPM_ST_SESSIONS:
	    break;
	  default:
	    rc = TPM_RC_BAD_TAG;
	}
    }
    return rc;
}