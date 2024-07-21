void rdp_read_flow_control_pdu(wStream* s, UINT16* type)
{
	/*
	 * Read flow control PDU - documented in FlowPDU section in T.128
	 * http://www.itu.int/rec/T-REC-T.128-199802-S/en
	 * The specification for the PDU has pad8bits listed BEFORE pduTypeFlow.
	 * However, so far pad8bits has always been observed to arrive AFTER pduTypeFlow.
	 * Switched the order of these two fields to match this observation.
	 */
	UINT8 pduType;
	Stream_Read_UINT8(s, pduType); /* pduTypeFlow */
	*type = pduType;
	Stream_Seek_UINT8(s);  /* pad8bits */
	Stream_Seek_UINT8(s);  /* flowIdentifier */
	Stream_Seek_UINT8(s);  /* flowNumber */
	Stream_Seek_UINT16(s); /* pduSource */
}