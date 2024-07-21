TPMI_RH_ENDORSEMENT_Unmarshal(TPMI_RH_ENDORSEMENT *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_RH_ENDORSEMENT:
	    break;
	  case TPM_RH_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_VALUE;
	}
    }
    return rc;
}