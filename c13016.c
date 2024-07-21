TPMI_DH_ENTITY_Unmarshal(TPMI_DH_ENTITY *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotOwner = *target != TPM_RH_OWNER;
	BOOL isNotEndorsement = *target != TPM_RH_ENDORSEMENT;
	BOOL isNotPlatform = *target != TPM_RH_PLATFORM;
	BOOL isNotLockout = *target != TPM_RH_LOCKOUT;
	BOOL isNotTransient = (*target < TRANSIENT_FIRST) || (*target > TRANSIENT_LAST);
	BOOL isNotPersistent = (*target < PERSISTENT_FIRST) || (*target > PERSISTENT_LAST);
	BOOL isNotNv = (*target < NV_INDEX_FIRST) || (*target > NV_INDEX_LAST);
	BOOL isNotPcr = (*target > PCR_LAST);
	BOOL isNotAuth = (*target < TPM_RH_AUTH_00) || (*target > TPM_RH_AUTH_FF);
	BOOL isNotLegalNull = (*target != TPM_RH_NULL) || !allowNull;
	if (isNotOwner &&
	    isNotEndorsement &&
	    isNotPlatform &&
	    isNotLockout &&
	    isNotTransient &&
	    isNotPersistent &&
	    isNotNv &&
	    isNotPcr &&
	    isNotAuth &&
	    isNotLegalNull) {
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}