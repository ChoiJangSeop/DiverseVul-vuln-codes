TPMI_YES_NO_Unmarshal(TPMI_YES_NO *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT8_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case NO:
	  case YES:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}