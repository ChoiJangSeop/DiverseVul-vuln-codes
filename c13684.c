TPM2B_Unmarshal(TPM2B *target, UINT16 targetSize, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(&target->size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->size > targetSize) {
	    rc = TPM_RC_SIZE;
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = Array_Unmarshal(target->buffer, target->size, buffer, size);
    }
    return rc;
}