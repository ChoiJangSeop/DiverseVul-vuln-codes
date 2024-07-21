TPMA_SESSION_Unmarshal(TPMA_SESSION *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT8_Unmarshal((UINT8 *)target, buffer, size);  /* libtpms changed */
    }
    if (rc == TPM_RC_SUCCESS) {
	if (*target & TPMA_SESSION_reserved) {
	    rc = TPM_RC_RESERVED_BITS;
	}
    }
    return rc;
}