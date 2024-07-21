TPM_GENERATED_Unmarshal(TPM_GENERATED *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	if (*target != TPM_GENERATED_VALUE) {
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}