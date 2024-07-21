bool DNP3_Base::ProcessData(int len, const u_char* data, bool orig)
	{
	Endpoint* endp = orig ? &orig_state : &resp_state;

	while ( len )
		{
		if ( endp->in_hdr )
			{
			// We're parsing the DNP3 header and link layer, get that in full.
			if ( ! AddToBuffer(endp, PSEUDO_APP_LAYER_INDEX, &data, &len) )
				return true;

			// The first two bytes must always be 0x0564.
			if( endp->buffer[0] != 0x05 || endp->buffer[1] != 0x64 )
				{
				analyzer->Weird("dnp3_header_lacks_magic");
				return false;
				}

			// Make sure header checksum is correct.
			if ( ! CheckCRC(PSEUDO_LINK_LAYER_LEN, endp->buffer, endp->buffer + PSEUDO_LINK_LAYER_LEN, "header") )
				{
				analyzer->ProtocolViolation("broken_checksum");
				return false;
				}

			// If the checksum works out, we're pretty certainly DNP3.
			analyzer->ProtocolConfirmation();

			// DNP3 packets without transport and application
			// layers can happen, we ignore them.
			if ( (endp->buffer[PSEUDO_LENGTH_INDEX] + 3) == (char)PSEUDO_LINK_LAYER_LEN  )
				{
				ClearEndpointState(orig);
				return true;
				}

			// Double check the direction in case the first
			// received packet is a response.
			u_char ctrl = endp->buffer[PSEUDO_CONTROL_FIELD_INDEX];

			if ( orig != (bool)(ctrl & 0x80) )
				analyzer->Weird("dnp3_unexpected_flow_direction");

			// Update state.
			endp->pkt_length = endp->buffer[PSEUDO_LENGTH_INDEX];
			endp->tpflags = endp->buffer[PSEUDO_TRANSPORT_INDEX];
			endp->in_hdr = false; // Now parsing application layer.

			// For the first packet, we submit the header to
			// BinPAC.
			if ( ++endp->pkt_cnt == 1 )
				interp->NewData(orig, endp->buffer, endp->buffer + PSEUDO_LINK_LAYER_LEN);
			}

		if ( ! endp->in_hdr )
			{
			assert(endp->pkt_length);

			// We're parsing the DNP3 application layer, get that
			// in full now as well. We calculate the number of
			// raw bytes the application layer consists of from
			// the packet length by determining how much 16-byte
			// chunks fit in there, and then add 2 bytes CRC for
			// each.
			int n = PSEUDO_APP_LAYER_INDEX + (endp->pkt_length - 5) + ((endp->pkt_length - 5) / 16) * 2
					+ 2 * ( ((endp->pkt_length - 5) % 16 == 0) ? 0 : 1) - 1 ;

			if ( ! AddToBuffer(endp, n, &data, &len) )
				return true;

			// Parse the the application layer data.
			if ( ! ParseAppLayer(endp) )
				return false;

			// Done with this packet, prepare for next.
			endp->buffer_len = 0;
			endp->in_hdr = true;
			}
		}

	return true;
	}