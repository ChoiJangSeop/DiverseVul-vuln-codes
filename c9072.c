void nego_process_negotiation_response(rdpNego* nego, wStream* s)
{
	UINT16 length;
	WLog_DBG(TAG, "RDP_NEG_RSP");

	if (Stream_GetRemainingLength(s) < 7)
	{
		WLog_ERR(TAG, "Invalid RDP_NEG_RSP");
		nego->state = NEGO_STATE_FAIL;
		return;
	}

	Stream_Read_UINT8(s, nego->flags);
	Stream_Read_UINT16(s, length);
	Stream_Read_UINT32(s, nego->SelectedProtocol);
	nego->state = NEGO_STATE_FINAL;
}