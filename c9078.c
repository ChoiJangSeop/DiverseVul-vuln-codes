int nego_recv(rdpTransport* transport, wStream* s, void* extra)
{
	BYTE li;
	BYTE type;
	UINT16 length;
	rdpNego* nego = (rdpNego*)extra;

	if (!tpkt_read_header(s, &length))
		return -1;

	if (!tpdu_read_connection_confirm(s, &li, length))
		return -1;

	if (li > 6)
	{
		/* rdpNegData (optional) */
		Stream_Read_UINT8(s, type); /* Type */

		switch (type)
		{
			case TYPE_RDP_NEG_RSP:
				nego_process_negotiation_response(nego, s);
				WLog_DBG(TAG, "selected_protocol: %" PRIu32 "", nego->SelectedProtocol);

				/* enhanced security selected ? */

				if (nego->SelectedProtocol)
				{
					if ((nego->SelectedProtocol == PROTOCOL_HYBRID) &&
					    (!nego->EnabledProtocols[PROTOCOL_HYBRID]))
					{
						nego->state = NEGO_STATE_FAIL;
					}

					if ((nego->SelectedProtocol == PROTOCOL_SSL) &&
					    (!nego->EnabledProtocols[PROTOCOL_SSL]))
					{
						nego->state = NEGO_STATE_FAIL;
					}
				}
				else if (!nego->EnabledProtocols[PROTOCOL_RDP])
				{
					nego->state = NEGO_STATE_FAIL;
				}

				break;

			case TYPE_RDP_NEG_FAILURE:
				nego_process_negotiation_failure(nego, s);
				break;
		}
	}
	else if (li == 6)
	{
		WLog_DBG(TAG, "no rdpNegData");

		if (!nego->EnabledProtocols[PROTOCOL_RDP])
			nego->state = NEGO_STATE_FAIL;
		else
			nego->state = NEGO_STATE_FINAL;
	}
	else
	{
		WLog_ERR(TAG, "invalid negotiation response");
		nego->state = NEGO_STATE_FAIL;
	}

	if (!tpkt_ensure_stream_consumed(s, length))
		return -1;
	return 0;
}