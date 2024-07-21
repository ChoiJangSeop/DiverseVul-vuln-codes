TPMI_TDES_KEY_BITS_Unmarshal(TPMI_SM4_KEY_BITS *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_KEY_BITS_Unmarshal(target, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case 128:
	  case 192:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}