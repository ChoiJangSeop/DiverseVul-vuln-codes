TPM2B_SENSITIVE_Unmarshal(TPM2B_SENSITIVE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    INT32 startSize;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(&target->size, buffer, size);
    }
    if (target->size != 0) {
	if (rc == TPM_RC_SUCCESS) {
	    startSize = *size;
	}
	if (rc == TPM_RC_SUCCESS) {
	    rc = TPMT_SENSITIVE_Unmarshal(&target->sensitiveArea, buffer, size);
	}
	if (rc == TPM_RC_SUCCESS) {
	    if (target->size != startSize - *size) {
		rc = TPM_RC_SIZE;
	    }
	}
    }
    return rc;
}