TPMI_SH_POLICY_Unmarshal(TPMI_SH_POLICY *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotPolicySession = (*target < POLICY_SESSION_FIRST) || (*target > POLICY_SESSION_LAST);
	if (isNotPolicySession) {
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}