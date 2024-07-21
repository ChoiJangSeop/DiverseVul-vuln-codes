TPML_DIGEST_Unmarshal(TPML_DIGEST *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    UINT32 i;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	/* TPM side is hard coded to 2 minimum */
	if (target->count < 2) {
	    rc = TPM_RC_SIZE;
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > 8) {
	    rc = TPM_RC_SIZE;
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPM2B_DIGEST_Unmarshal(&target->digests[i], buffer, size);
    }
    return rc;
}