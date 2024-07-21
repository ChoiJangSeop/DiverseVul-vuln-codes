TPMI_RH_AC_Unmarshal(TPMI_RH_AC *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotAC = (*target < AC_FIRST) || (*target > AC_LAST);
	if (isNotAC) {
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}