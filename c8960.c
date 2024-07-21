BOOL rdp_read_share_control_header(wStream* s, UINT16* length, UINT16* type, UINT16* channel_id)
{
	if (Stream_GetRemainingLength(s) < 2)
		return FALSE;

	/* Share Control Header */
	Stream_Read_UINT16(s, *length); /* totalLength */

	/* If length is 0x8000 then we actually got a flow control PDU that we should ignore
	 http://msdn.microsoft.com/en-us/library/cc240576.aspx */
	if (*length == 0x8000)
	{
		rdp_read_flow_control_pdu(s, type);
		*channel_id = 0;
		*length = 8; /* Flow control PDU is 8 bytes */
		return TRUE;
	}

	if (((size_t)*length - 2) > Stream_GetRemainingLength(s))
		return FALSE;

	Stream_Read_UINT16(s, *type); /* pduType */
	*type &= 0x0F;                /* type is in the 4 least significant bits */

	if (*length > 4)
		Stream_Read_UINT16(s, *channel_id); /* pduSource */
	else
		*channel_id = 0; /* Windows XP can send such short DEACTIVATE_ALL PDUs. */

	return TRUE;
}