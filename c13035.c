TPMI_DH_PARENT_Unmarshal(TPMI_DH_PARENT *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotTransient = (*target < TRANSIENT_FIRST) || (*target > TRANSIENT_LAST);
	BOOL isNotPersistent = (*target < PERSISTENT_FIRST) || (*target > PERSISTENT_LAST);
	BOOL isNotOwner = *target != TPM_RH_OWNER;
	BOOL isNotPlatform = *target != TPM_RH_PLATFORM;
	BOOL isNotEndorsement = *target != TPM_RH_ENDORSEMENT;
	BOOL isNotLegalNull = (*target != TPM_RH_NULL) || !allowNull;
	if (isNotTransient &&
	    isNotPersistent &&
	    isNotOwner &&
	    isNotPlatform &&
	    isNotEndorsement && 
	    isNotLegalNull) {
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}