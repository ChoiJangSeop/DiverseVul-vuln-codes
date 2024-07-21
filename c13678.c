TPMS_NV_PUBLIC_Unmarshal(TPMS_NV_PUBLIC *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_RH_NV_INDEX_Unmarshal(&target->nvIndex, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_HASH_Unmarshal(&target->nameAlg, buffer, size, NO);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMA_NV_Unmarshal(&target->attributes, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->authPolicy, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(&target->dataSize, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->dataSize > MAX_NV_INDEX_SIZE) {
	    rc = TPM_RC_SIZE;
	}
    }
    return rc;
}