TPMI_DH_PCR_Unmarshal(TPMI_DH_PCR *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotPcr = (*target > PCR_LAST);
	BOOL isNotLegalNull = (*target != TPM_RH_NULL) || !allowNull;
	if (isNotPcr &&
	    isNotLegalNull) {
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}