TPM_EO_Unmarshal(TPM_EO *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_EO_EQ:
	  case TPM_EO_NEQ:
	  case TPM_EO_SIGNED_GT:
	  case TPM_EO_UNSIGNED_GT:
	  case TPM_EO_SIGNED_LT:
	  case TPM_EO_UNSIGNED_LT:
	  case TPM_EO_SIGNED_GE:
	  case TPM_EO_UNSIGNED_GE:
	  case TPM_EO_SIGNED_LE:
	  case TPM_EO_UNSIGNED_LE:
	  case TPM_EO_BITSET:
	  case TPM_EO_BITCLEAR:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}