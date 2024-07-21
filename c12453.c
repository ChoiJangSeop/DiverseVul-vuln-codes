bool DNP3_Base::ParseAppLayer(Endpoint* endp)
	{
	bool orig = (endp == &orig_state);
	binpac::DNP3::DNP3_Flow* flow = orig ? interp->upflow() : interp->downflow();

	u_char* data = endp->buffer + PSEUDO_TRANSPORT_INDEX; // The transport layer byte counts as app-layer it seems.
	int len = endp->pkt_length - 5;

	// DNP3 Packet :  DNP3 Pseudo Link Layer | DNP3 Pseudo Transport Layer | DNP3 Pseudo Application Layer
	// DNP3 Serial Transport Layer data is always 1 byte.
	// Get FIN FIR seq field in transport header.
	// FIR indicate whether the following DNP3 Serial Application Layer is first chunk of bytes or not.
	// FIN indicate whether the following DNP3 Serial Application Layer is last chunk of bytes or not.

	int is_first = (endp->tpflags & 0x40) >> 6; // Initial chunk of data in this packet.
	int is_last = (endp->tpflags & 0x80) >> 7; // Last chunk of data in this packet.

	int transport = PSEUDO_TRANSPORT_LEN;

	int i = 0;
	while ( len > 0 )
		{
		int n = min(len, 16);

		// Make sure chunk has a correct checksum.
		if ( ! CheckCRC(n, data, data + n, "app_chunk") )
			return false;

		// Pass on to BinPAC.
		assert(data + n < endp->buffer + endp->buffer_len);
		flow->flow_buffer()->BufferData(data + transport, data + n);
		transport = 0;

		data += n + 2;
		len -= n;
		}

	if ( is_first )
		endp->encountered_first_chunk = true;

	if ( ! is_first && ! endp->encountered_first_chunk )
		{
		// We lost the first chunk.
		analyzer->Weird("dnp3_first_application_layer_chunk_missing");
		return false;
		}

	if ( is_last )
		{
		flow->flow_buffer()->FinishBuffer();
		flow->FlowEOF();
		ClearEndpointState(orig);
		}

	return true;
	}