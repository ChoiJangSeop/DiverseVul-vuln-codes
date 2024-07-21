void nego_process_negotiation_request(rdpNego* nego, wStream* s)
{
	BYTE flags;
	UINT16 length;
	Stream_Read_UINT8(s, flags);
	Stream_Read_UINT16(s, length);
	Stream_Read_UINT32(s, nego->RequestedProtocols);
	WLog_DBG(TAG, "RDP_NEG_REQ: RequestedProtocol: 0x%08" PRIX32 "", nego->RequestedProtocols);
	nego->state = NEGO_STATE_FINAL;
}