TPML_ALG_Unmarshal(TPML_ALG *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    UINT32 i;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > MAX_ALG_LIST_SIZE) {
	    rc = TPM_RC_SIZE;
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPM_ALG_ID_Unmarshal(&target->algorithms[i], buffer, size);
    }
    return rc;
}