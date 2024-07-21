TPM_CAP_Unmarshal(TPM_CAP *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_CAP_ALGS:
	  case TPM_CAP_HANDLES:
	  case TPM_CAP_COMMANDS:
	  case TPM_CAP_PP_COMMANDS:
	  case TPM_CAP_AUDIT_COMMANDS:
	  case TPM_CAP_PCRS:
	  case TPM_CAP_TPM_PROPERTIES:
	  case TPM_CAP_PCR_PROPERTIES:
	  case TPM_CAP_ECC_CURVES:
	  case TPM_CAP_AUTH_POLICIES:
	  case TPM_CAP_ACT:
	  case TPM_CAP_VENDOR_PROPERTY:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}