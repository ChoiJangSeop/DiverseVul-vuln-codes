int64 CSteamNetworkConnectionBase::SNP_SendMessage( CSteamNetworkingMessage *pSendMessage, SteamNetworkingMicroseconds usecNow, bool *pbThinkImmediately )
{
	int cbData = (int)pSendMessage->m_cbSize;

	// Assume we won't want to wake up immediately
	if ( pbThinkImmediately )
		*pbThinkImmediately = false;

	// Check if we're full
	if ( m_senderState.PendingBytesTotal() + cbData > m_connectionConfig.m_SendBufferSize.Get() )
	{
		SpewWarningRateLimited( usecNow, "Connection already has %u bytes pending, cannot queue any more messages\n", m_senderState.PendingBytesTotal() );
		pSendMessage->Release();
		return -k_EResultLimitExceeded; 
	}

	// Check if they try to send a really large message
	if ( cbData > k_cbMaxUnreliableMsgSize && !( pSendMessage->m_nFlags & k_nSteamNetworkingSend_Reliable )  )
	{
		SpewWarningRateLimited( usecNow, "Trying to send a very large (%d bytes) unreliable message.  Sending as reliable instead.\n", cbData );
		pSendMessage->m_nFlags |= k_nSteamNetworkingSend_Reliable;
	}

	if ( pSendMessage->m_nFlags & k_nSteamNetworkingSend_NoDelay )
	{
		// FIXME - need to check how much data is currently pending, and return
		// k_EResultIgnored if we think it's going to be a while before this
		// packet goes on the wire.
	}

	// First, accumulate tokens, and also limit to reasonable burst
	// if we weren't already waiting to send
	SNP_ClampSendRate();
	SNP_TokenBucket_Accumulate( usecNow );

	// Assign a message number
	pSendMessage->m_nMessageNumber = ++m_senderState.m_nLastSentMsgNum;

	// Reliable, or unreliable?
	if ( pSendMessage->m_nFlags & k_nSteamNetworkingSend_Reliable )
	{
		pSendMessage->SNPSend_SetReliableStreamPos( m_senderState.m_nReliableStreamPos );

		// Generate the header
		byte *hdr = pSendMessage->SNPSend_ReliableHeader();
		hdr[0] = 0;
		byte *hdrEnd = hdr+1;
		int64 nMsgNumGap = pSendMessage->m_nMessageNumber - m_senderState.m_nLastSendMsgNumReliable;
		Assert( nMsgNumGap >= 1 );
		if ( nMsgNumGap > 1 )
		{
			hdrEnd = SerializeVarInt( hdrEnd, (uint64)nMsgNumGap );
			hdr[0] |= 0x40;
		}
		if ( cbData < 0x20 )
		{
			hdr[0] |= (byte)cbData;
		}
		else
		{
			hdr[0] |= (byte)( 0x20 | ( cbData & 0x1f ) );
			hdrEnd = SerializeVarInt( hdrEnd, cbData>>5U );
		}
		pSendMessage->m_cbSNPSendReliableHeader = hdrEnd - hdr;

		// Grow the total size of the message by the header
		pSendMessage->m_cbSize += pSendMessage->m_cbSNPSendReliableHeader;

		// Advance stream pointer
		m_senderState.m_nReliableStreamPos += pSendMessage->m_cbSize;

		// Update stats
		++m_senderState.m_nMessagesSentReliable;
		m_senderState.m_cbPendingReliable += pSendMessage->m_cbSize;

		// Remember last sent reliable message number, so we can know how to
		// encode the next one
		m_senderState.m_nLastSendMsgNumReliable = pSendMessage->m_nMessageNumber;

		Assert( pSendMessage->SNPSend_IsReliable() );
	}
	else
	{
		pSendMessage->SNPSend_SetReliableStreamPos( 0 );
		pSendMessage->m_cbSNPSendReliableHeader = 0;

		++m_senderState.m_nMessagesSentUnreliable;
		m_senderState.m_cbPendingUnreliable += pSendMessage->m_cbSize;

		Assert( !pSendMessage->SNPSend_IsReliable() );
	}

	// Add to pending list
	m_senderState.m_messagesQueued.push_back( pSendMessage );
	SpewVerboseGroup( m_connectionConfig.m_LogLevel_Message.Get(), "[%s] SendMessage %s: MsgNum=%lld sz=%d\n",
				 GetDescription(),
				 pSendMessage->SNPSend_IsReliable() ? "RELIABLE" : "UNRELIABLE",
				 (long long)pSendMessage->m_nMessageNumber,
				 pSendMessage->m_cbSize );

	// Use Nagle?
	// We always set the Nagle timer, even if we immediately clear it.  This makes our clearing code simpler,
	// since we can always safely assume that once we find a message with the nagle timer cleared, all messages
	// queued earlier than this also have it cleared.
	// FIXME - Don't think this works if the configuration value is changing.  Since changing the
	// config value could violate the assumption that nagle times are increasing.  Probably not worth
	// fixing.
	pSendMessage->SNPSend_SetUsecNagle( usecNow + m_connectionConfig.m_NagleTime.Get() );
	if ( pSendMessage->m_nFlags & k_nSteamNetworkingSend_NoNagle )
		m_senderState.ClearNagleTimers();

	// Save the message number.  The code below might end up deleting the message we just queued
	int64 result = pSendMessage->m_nMessageNumber;

	// Schedule wakeup at the appropriate time.  (E.g. right now, if we're ready to send, 
	// or at the Nagle time, if Nagle is active.)
	//
	// NOTE: Right now we might not actually be capable of sending end to end data.
	// But that case is relatievly rare, and nothing will break if we try to right now.
	// On the other hand, just asking the question involved a virtual function call,
	// and it will return success most of the time, so let's not make the check here.
	if ( GetState() == k_ESteamNetworkingConnectionState_Connected )
	{
		SteamNetworkingMicroseconds usecNextThink = SNP_GetNextThinkTime( usecNow );

		// Ready to send now?
		if ( usecNextThink > usecNow )
		{

			// We are rate limiting.  Spew about it?
			if ( m_senderState.m_messagesQueued.m_pFirst->SNPSend_UsecNagle() == 0 )
			{
				SpewVerbose( "[%s] RATELIM QueueTime is %.1fms, SendRate=%.1fk, BytesQueued=%d\n", 
					GetDescription(),
					m_senderState.CalcTimeUntilNextSend() * 1e-3,
					m_senderState.m_n_x * ( 1.0/1024.0),
					m_senderState.PendingBytesTotal()
				);
			}

			// Set a wakeup call.
			EnsureMinThinkTime( usecNextThink );
		}
		else
		{

			// We're ready to send right now.  Check if we should!
			if ( pSendMessage->m_nFlags & k_nSteamNetworkingSend_UseCurrentThread )
			{

				// We should send in this thread, before the API entry point
				// that the app used returns.  Is the caller gonna handle this?
				if ( pbThinkImmediately )
				{
					// Caller says they will handle it
					*pbThinkImmediately = true;
				}
				else
				{
					// Caller wants us to just do it here.
					CheckConnectionStateAndSetNextThinkTime( usecNow );
				}
			}
			else
			{
				// Wake up the service thread ASAP to send this in the background thread
				SetNextThinkTimeASAP();
			}
		}
	}

	return result;
}