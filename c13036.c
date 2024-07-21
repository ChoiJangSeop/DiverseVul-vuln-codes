TPMI_DH_CONTEXT_Unmarshal(TPMI_DH_CONTEXT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotHmacSession = (*target < HMAC_SESSION_FIRST ) || (*target > HMAC_SESSION_LAST);
	BOOL isNotPolicySession = (*target < POLICY_SESSION_FIRST) || (*target > POLICY_SESSION_LAST);
	BOOL isNotTransient = (*target < TRANSIENT_FIRST) || (*target > TRANSIENT_LAST);
	if (isNotHmacSession &&
	    isNotPolicySession &&
	    isNotTransient) {
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}