TPMI_RH_HIERARCHY_POLICY_Unmarshal(TPMI_RH_HIERARCHY_POLICY *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_RH_OWNER:
	  case TPM_RH_PLATFORM:
	  case TPM_RH_ENDORSEMENT:
	  case TPM_RH_LOCKOUT:
	    break;
	  default:
	      {
		  BOOL isNotHP =  (*target < TPM_RH_ACT_0) || (*target > TPM_RH_ACT_F);
		  if (isNotHP) {
		      rc = TPM_RC_VALUE;
		  }
	      }
	}
    }
    return rc;
}