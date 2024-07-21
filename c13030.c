TPMI_DH_SAVED_Unmarshal(TPMI_DH_SAVED *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    allowNull = allowNull;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotHmacSession = (*target < HMAC_SESSION_FIRST ) || (*target > HMAC_SESSION_LAST);
	BOOL isNotPolicySession = (*target < POLICY_SESSION_FIRST) || (*target > POLICY_SESSION_LAST);
	BOOL isNotTransientObject = (*target != 0x80000000);
	BOOL isNotSequenceObject = (*target != 0x80000001);
	BOOL isNotTransientStClear = (*target != 0x80000002);
	if (isNotHmacSession &&
	    isNotPolicySession &&
	    isNotTransientObject &&
	    isNotSequenceObject &&
	    isNotTransientStClear) {
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}