TPML_TAGGED_POLICY_Unmarshal(TPML_TAGGED_POLICY *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    UINT32 i;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > MAX_TAGGED_POLICIES) {
	    rc = TPM_RC_SIZE;
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPMS_TAGGED_POLICY_Unmarshal(&target->policies[i], buffer, size);
    }
    return rc;
}