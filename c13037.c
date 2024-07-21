TPMI_DH_OBJECT_Unmarshal(TPMI_DH_OBJECT *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotTransient = (*target < TRANSIENT_FIRST) || (*target > TRANSIENT_LAST);
	BOOL isNotPersistent = (*target < PERSISTENT_FIRST) || (*target > PERSISTENT_LAST);
	BOOL isNotLegalNull = (*target != TPM_RH_NULL) || !allowNull;
	if (isNotTransient &&
	    isNotPersistent &&
	    isNotLegalNull) {
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}