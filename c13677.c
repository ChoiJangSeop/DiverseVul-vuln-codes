TPM2B_ECC_POINT_Unmarshal(TPM2B_ECC_POINT *target, BYTE **buffer, INT32 *size)
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
	rc = TPMS_ECC_POINT_Unmarshal(&target->point, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->size != startSize - *size) {
	    rc = TPM_RC_SIZE;
	}
    }
    return rc;
}