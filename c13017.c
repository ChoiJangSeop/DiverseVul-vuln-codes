TPMI_SH_AUTH_SESSION_Unmarshal(TPMI_SH_AUTH_SESSION *target, BYTE **buffer, INT32 *size, BOOL allowPwd)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotHmacSession = (*target < HMAC_SESSION_FIRST ) || (*target > HMAC_SESSION_LAST);
	BOOL isNotPolicySession = (*target < POLICY_SESSION_FIRST) || (*target > POLICY_SESSION_LAST);
	BOOL isNotLegalPwd = (*target != TPM_RS_PW) || !allowPwd;
	if (isNotHmacSession &&
	    isNotPolicySession &&
	    isNotLegalPwd) {
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}