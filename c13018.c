TPMI_DH_PERSISTENT_Unmarshal(TPMI_DH_PERSISTENT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotPersistent = (*target < PERSISTENT_FIRST) || (*target > PERSISTENT_LAST);
	if (isNotPersistent) {
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}