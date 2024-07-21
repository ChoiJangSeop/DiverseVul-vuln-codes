TPMS_PCR_SELECTION_Unmarshal(TPMS_PCR_SELECTION *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_HASH_Unmarshal(&target->hash, buffer, size, NO);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT8_Unmarshal(&target->sizeofSelect, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if ((target->sizeofSelect < PCR_SELECT_MIN) ||
	    (target->sizeofSelect > PCR_SELECT_MAX)) {
	    rc = TPM_RC_VALUE;
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = Array_Unmarshal(target->pcrSelect, target->sizeofSelect, buffer, size);
    }
    return rc;
}