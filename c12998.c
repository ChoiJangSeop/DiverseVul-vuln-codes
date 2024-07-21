TPMT_TK_AUTH_Unmarshal(TPMT_TK_AUTH *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ST_Unmarshal(&target->tag, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if ((target->tag != TPM_ST_AUTH_SIGNED) &&
	    (target->tag != TPM_ST_AUTH_SECRET)) {
	    rc = TPM_RC_TAG;
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_RH_HIERARCHY_Unmarshal(&target->hierarchy, buffer, size, YES);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->digest, buffer, size);
    }
    return rc;
}