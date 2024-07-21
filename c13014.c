TPMI_ECC_CURVE_Unmarshal(TPMI_ECC_CURVE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ECC_CURVE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ECC_BN_P256
	  case TPM_ECC_BN_P256:
#endif
#if ECC_BN_P638		// libtpms added begin
	  case TPM_ECC_BN_P638:
#endif
#if ECC_NIST_P192
	  case TPM_ECC_NIST_P192:
#endif
#if ECC_NIST_P224
	  case TPM_ECC_NIST_P224:
#endif			// libtpms added end
#if ECC_NIST_P256
	  case TPM_ECC_NIST_P256:
#endif
#if ECC_NIST_P384
	  case TPM_ECC_NIST_P384:
#endif
#if ECC_NIST_P521	// libtpms added begin
	  case TPM_ECC_NIST_P521:
#endif
#if ECC_SM2_P256
	  case TPM_ECC_SM2_P256:
#endif
          if (!CryptEccIsCurveRuntimeUsable(*target))
              rc = TPM_RC_CURVE;
                      // libtpms added end
	    break;
	  default:
	    rc = TPM_RC_CURVE;
	}
    }
    return rc;
}