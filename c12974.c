TPMI_RH_ACT_Unmarshal( TPMI_RH_ACT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotACT = (*target < TPM_RH_ACT_0) || (*target > TPM_RH_ACT_F);
	if (isNotACT) {
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}