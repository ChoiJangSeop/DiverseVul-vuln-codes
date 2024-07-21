void nego_process_negotiation_failure(rdpNego* nego, wStream* s)
{
	BYTE flags;
	UINT16 length;
	UINT32 failureCode;
	WLog_DBG(TAG, "RDP_NEG_FAILURE");
	Stream_Read_UINT8(s, flags);
	Stream_Read_UINT16(s, length);
	Stream_Read_UINT32(s, failureCode);

	switch (failureCode)
	{
		case SSL_REQUIRED_BY_SERVER:
			WLog_WARN(TAG, "Error: SSL_REQUIRED_BY_SERVER");
			break;

		case SSL_NOT_ALLOWED_BY_SERVER:
			WLog_WARN(TAG, "Error: SSL_NOT_ALLOWED_BY_SERVER");
			nego->sendNegoData = TRUE;
			break;

		case SSL_CERT_NOT_ON_SERVER:
			WLog_ERR(TAG, "Error: SSL_CERT_NOT_ON_SERVER");
			nego->sendNegoData = TRUE;
			break;

		case INCONSISTENT_FLAGS:
			WLog_ERR(TAG, "Error: INCONSISTENT_FLAGS");
			break;

		case HYBRID_REQUIRED_BY_SERVER:
			WLog_WARN(TAG, "Error: HYBRID_REQUIRED_BY_SERVER");
			break;

		default:
			WLog_ERR(TAG, "Error: Unknown protocol security error %" PRIu32 "", failureCode);
			break;
	}

	nego->state = NEGO_STATE_FAIL;
}