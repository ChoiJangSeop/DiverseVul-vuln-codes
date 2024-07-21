TPMA_CC_Unmarshal(TPMA_CC *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal((UINT32 *)target, buffer, size); /* libtpms changed */
    }
    if (rc == TPM_RC_SUCCESS) {
	if (*target & TPMA_CC_reserved) {
	    rc = TPM_RC_RESERVED_BITS;
	}
    }
    return rc;
}