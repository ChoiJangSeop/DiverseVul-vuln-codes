TPM_CLOCK_ADJUST_Unmarshal(TPM_CLOCK_ADJUST *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = INT8_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_CLOCK_COARSE_SLOWER:
	  case TPM_CLOCK_MEDIUM_SLOWER:
	  case TPM_CLOCK_FINE_SLOWER:
	  case TPM_CLOCK_NO_CHANGE:
	  case TPM_CLOCK_FINE_FASTER:
	  case TPM_CLOCK_MEDIUM_FASTER:
	  case TPM_CLOCK_COARSE_FASTER:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}