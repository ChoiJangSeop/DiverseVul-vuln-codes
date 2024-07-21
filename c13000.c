TPMI_RH_NV_AUTH_Unmarshal(TPMI_RH_NV_AUTH *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_RH_OWNER:
	  case TPM_RH_PLATFORM:
	    break;
	  default:
	      {
		  BOOL isNotNv = (*target < NV_INDEX_FIRST) || (*target > NV_INDEX_LAST);
		  if (isNotNv) {
		      rc = TPM_RC_VALUE;
		  }
	      }
	}
    }
    return rc;
}