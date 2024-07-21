TPM2B_PUBLIC_Unmarshal(TPM2B_PUBLIC *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    INT32 startSize;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(&target->size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->size == 0) {
	    rc = TPM_RC_SIZE;
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	startSize = *size;
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMT_PUBLIC_Unmarshal(&target->publicArea, buffer, size, allowNull);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->size != startSize - *size) {
	    rc = TPM_RC_SIZE;
	}
    }
    return rc;
}