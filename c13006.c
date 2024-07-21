TPM_ECC_CURVE_Unmarshal(TPM_ECC_CURVE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(target, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_ECC_NONE:
	  case TPM_ECC_NIST_P192:
	  case TPM_ECC_NIST_P224:
	  case TPM_ECC_NIST_P256:
	  case TPM_ECC_NIST_P384:
	  case TPM_ECC_NIST_P521:
	  case TPM_ECC_BN_P256:
	  case TPM_ECC_BN_P638:
	  case TPM_ECC_SM2_P256:
	    break;
	  default:
	    rc = TPM_RC_CURVE;
	}
    }
    return rc;
}