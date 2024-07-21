bool CSteamNetworkConnectionBase::ProcessPlainTextDataChunk( int usecTimeSinceLast, RecvPacketContext_t &ctx )
{
	#define DECODE_ERROR( ... ) do { \
		ConnectionState_ProblemDetectedLocally( k_ESteamNetConnectionEnd_Misc_InternalError, __VA_ARGS__ ); \
		return false; } while(false)

	#define EXPECT_BYTES(n,pszWhatFor) \
		do { \
			if ( pDecode + (n) > pEnd ) \
				DECODE_ERROR( "SNP decode overrun, %d bytes for %s", (n), pszWhatFor ); \
		} while (false)

	#define READ_8BITU( var, pszWhatFor ) \
		do { EXPECT_BYTES(1,pszWhatFor); var = *(uint8 *)pDecode; pDecode += 1; } while(false)

	#define READ_16BITU( var, pszWhatFor ) \
		do { EXPECT_BYTES(2,pszWhatFor); var = LittleWord(*(uint16 *)pDecode); pDecode += 2; } while(false)

	#define READ_24BITU( var, pszWhatFor ) \
		do { EXPECT_BYTES(3,pszWhatFor); \
			var = *(uint8 *)pDecode; pDecode += 1; \
			var |= uint32( LittleWord(*(uint16 *)pDecode) ) << 8U; pDecode += 2; \
		} while(false)

	#define READ_32BITU( var, pszWhatFor ) \
		do { EXPECT_BYTES(4,pszWhatFor); var = LittleDWord(*(uint32 *)pDecode); pDecode += 4; } while(false)

	#define READ_48BITU( var, pszWhatFor ) \
		do { EXPECT_BYTES(6,pszWhatFor); \
			var = LittleWord( *(uint16 *)pDecode ); pDecode += 2; \
			var |= uint64( LittleDWord(*(uint32 *)pDecode) ) << 16U; pDecode += 4; \
		} while(false)

	#define READ_64BITU( var, pszWhatFor ) \
		do { EXPECT_BYTES(8,pszWhatFor); var = LittleQWord(*(uint64 *)pDecode); pDecode += 8; } while(false)

	#define READ_VARINT( var, pszWhatFor ) \
		do { pDecode = DeserializeVarInt( pDecode, pEnd, var ); if ( !pDecode ) { DECODE_ERROR( "SNP data chunk decode overflow, varint for %s", pszWhatFor ); } } while(false)

	#define READ_SEGMENT_DATA_SIZE( is_reliable ) \
		int cbSegmentSize; \
		{ \
			int sizeFlags = nFrameType & 7; \
			if ( sizeFlags <= 4 ) \
			{ \
				uint8 lowerSizeBits; \
				READ_8BITU( lowerSizeBits, #is_reliable " size lower bits" ); \
				cbSegmentSize = (sizeFlags<<8) + lowerSizeBits; \
				if ( pDecode + cbSegmentSize > pEnd ) \
				{ \
					DECODE_ERROR( "SNP decode overrun %d bytes for %s segment data.", cbSegmentSize, #is_reliable ); \
				} \
			} \
			else if ( sizeFlags == 7 ) \
			{ \
				cbSegmentSize = pEnd - pDecode; \
			} \
			else \
			{ \
				DECODE_ERROR( "Invalid SNP frame lead byte 0x%02x. (size bits)", nFrameType ); \
			} \
		} \
		const uint8 *pSegmentData = pDecode; \
		pDecode += cbSegmentSize;

	// Make sure we have initialized the connection
	Assert( BStateIsActive() );

	const SteamNetworkingMicroseconds usecNow = ctx.m_usecNow;
	const int64 nPktNum = ctx.m_nPktNum;
	bool bInhibitMarkReceived = false;

	const int nLogLevelPacketDecode = m_connectionConfig.m_LogLevel_PacketDecode.Get();
	SpewVerboseGroup( nLogLevelPacketDecode, "[%s] decode pkt %lld\n", GetDescription(), (long long)nPktNum );

	// Decode frames until we get to the end of the payload
	const byte *pDecode = (const byte *)ctx.m_pPlainText;
	const byte *pEnd = pDecode + ctx.m_cbPlainText;
	int64 nCurMsgNum = 0;
	int64 nDecodeReliablePos = 0;
	while ( pDecode < pEnd )
	{

		uint8 nFrameType = *pDecode;
		++pDecode;
		if ( ( nFrameType & 0xc0 ) == 0x00 )
		{

			//
			// Unreliable segment
			//

			// Decode message number
			if ( nCurMsgNum == 0 )
			{
				// First unreliable frame.  Message number is absolute, but only bottom N bits are sent
				static const char szUnreliableMsgNumOffset[] = "unreliable msgnum";
				int64 nLowerBits, nMask;
				if ( nFrameType & 0x10 )
				{
					READ_32BITU( nLowerBits, szUnreliableMsgNumOffset );
					nMask = 0xffffffff;
					nCurMsgNum = NearestWithSameLowerBits( (int32)nLowerBits, m_receiverState.m_nHighestSeenMsgNum );
				}
				else
				{
					READ_16BITU( nLowerBits, szUnreliableMsgNumOffset );
					nMask = 0xffff;
					nCurMsgNum = NearestWithSameLowerBits( (int16)nLowerBits, m_receiverState.m_nHighestSeenMsgNum );
				}
				Assert( ( nCurMsgNum & nMask ) == nLowerBits );

				if ( nCurMsgNum <= 0 )
				{
					DECODE_ERROR( "SNP decode unreliable msgnum underflow.  %llx mod %llx, highest seen %llx",
						(unsigned long long)nLowerBits, (unsigned long long)( nMask+1 ), (unsigned long long)m_receiverState.m_nHighestSeenMsgNum );
				}
				if ( std::abs( nCurMsgNum - m_receiverState.m_nHighestSeenMsgNum ) > (nMask>>2) )
				{
					// We really should never get close to this boundary.
					SpewWarningRateLimited( usecNow, "Sender sent abs unreliable message number using %llx mod %llx, highest seen %llx\n",
						(unsigned long long)nLowerBits, (unsigned long long)( nMask+1 ), (unsigned long long)m_receiverState.m_nHighestSeenMsgNum );
				}

			}
			else
			{
				if ( nFrameType & 0x10 )
				{
					uint64 nMsgNumOffset;
					READ_VARINT( nMsgNumOffset, "unreliable msgnum offset" );
					nCurMsgNum += nMsgNumOffset;
				}
				else
				{
					++nCurMsgNum;
				}
			}
			if ( nCurMsgNum > m_receiverState.m_nHighestSeenMsgNum )
				m_receiverState.m_nHighestSeenMsgNum = nCurMsgNum;

			//
			// Decode segment offset in message
			//
			uint32 nOffset = 0;
			if ( nFrameType & 0x08 )
				READ_VARINT( nOffset, "unreliable data offset" );

			//
			// Decode size, locate segment data
			//
			READ_SEGMENT_DATA_SIZE( unreliable )
			Assert( cbSegmentSize > 0 ); // !TEST! Bogus assert, zero byte messages are OK.  Remove after testing

			// Receive the segment
			bool bLastSegmentInMessage = ( nFrameType & 0x20 ) != 0;
			SNP_ReceiveUnreliableSegment( nCurMsgNum, nOffset, pSegmentData, cbSegmentSize, bLastSegmentInMessage, usecNow );
		}
		else if ( ( nFrameType & 0xe0 ) == 0x40 )
		{

			//
			// Reliable segment
			//

			// First reliable segment?
			if ( nDecodeReliablePos == 0 )
			{

				// Stream position is absolute.  How many bits?
				static const char szFirstReliableStreamPos[] = "first reliable streampos";
				int64 nOffset, nMask;
				switch ( nFrameType & (3<<3) )
				{
					case 0<<3: READ_24BITU( nOffset, szFirstReliableStreamPos ); nMask = (1ll<<24)-1; break;
					case 1<<3: READ_32BITU( nOffset, szFirstReliableStreamPos ); nMask = (1ll<<32)-1; break;
					case 2<<3: READ_48BITU( nOffset, szFirstReliableStreamPos ); nMask = (1ll<<48)-1; break;
					default: DECODE_ERROR( "Reserved reliable stream pos size" );
				}

				// What do we expect to receive next?
				int64 nExpectNextStreamPos = m_receiverState.m_nReliableStreamPos + len( m_receiverState.m_bufReliableStream );

				// Find the stream offset closest to that
				nDecodeReliablePos = ( nExpectNextStreamPos & ~nMask ) + nOffset;
				if ( nDecodeReliablePos + (nMask>>1) < nExpectNextStreamPos )
				{
					nDecodeReliablePos += nMask+1;
					Assert( ( nDecodeReliablePos & nMask ) == nOffset );
					Assert( nExpectNextStreamPos < nDecodeReliablePos );
					Assert( nExpectNextStreamPos + (nMask>>1) >= nDecodeReliablePos );
				}
				if ( nDecodeReliablePos <= 0 )
				{
					DECODE_ERROR( "SNP decode first reliable stream pos underflow.  %llx mod %llx, expected next %llx",
						(unsigned long long)nOffset, (unsigned long long)( nMask+1 ), (unsigned long long)nExpectNextStreamPos );
				}
				if ( std::abs( nDecodeReliablePos - nExpectNextStreamPos ) > (nMask>>2) )
				{
					// We really should never get close to this boundary.
					SpewWarningRateLimited( usecNow, "Sender sent reliable stream pos using %llx mod %llx, expected next %llx\n",
						(unsigned long long)nOffset, (unsigned long long)( nMask+1 ), (unsigned long long)nExpectNextStreamPos );
				}
			}
			else
			{
				// Subsequent reliable message encode the position as an offset from previous.
				static const char szOtherReliableStreamPos[] = "reliable streampos offset";
				int64 nOffset;
				switch ( nFrameType & (3<<3) )
				{
					case 0<<3: nOffset = 0; break;
					case 1<<3: READ_8BITU( nOffset, szOtherReliableStreamPos ); break;
					case 2<<3: READ_16BITU( nOffset, szOtherReliableStreamPos ); break;
					default: READ_32BITU( nOffset, szOtherReliableStreamPos ); break;
				}
				nDecodeReliablePos += nOffset;
			}

			//
			// Decode size, locate segment data
			//
			READ_SEGMENT_DATA_SIZE( reliable )

			// Ingest the segment.
			if ( !SNP_ReceiveReliableSegment( nPktNum, nDecodeReliablePos, pSegmentData, cbSegmentSize, usecNow ) )
			{
				if ( !BStateIsActive() )
					return false; // we decided to nuke the connection - abort packet processing

				// We're not able to ingest this reliable segment at the moment,
				// but we didn't terminate the connection.  So do not ack this packet
				// to the peer.  We need them to retransmit
				bInhibitMarkReceived = true;
			}

			// Advance pointer for the next reliable segment, if any.
			nDecodeReliablePos += cbSegmentSize;

			// Decoding rules state that if we have established a message number,
			// (from an earlier unreliable message), then we advance it.
			if ( nCurMsgNum > 0 ) 
				++nCurMsgNum;
		}
		else if ( ( nFrameType & 0xfc ) == 0x80 )
		{
			//
			// Stop waiting
			//

			int64 nOffset = 0;
			static const char szStopWaitingOffset[] = "stop_waiting offset";
			switch ( nFrameType & 3 )
			{
				case 0: READ_8BITU( nOffset, szStopWaitingOffset ); break;
				case 1: READ_16BITU( nOffset, szStopWaitingOffset ); break;
				case 2: READ_24BITU( nOffset, szStopWaitingOffset ); break;
				case 3: READ_64BITU( nOffset, szStopWaitingOffset ); break;
			}
			if ( nOffset >= nPktNum )
			{
				DECODE_ERROR( "stop_waiting pktNum %llu offset %llu", nPktNum, nOffset );
			}
			++nOffset;
			int64 nMinPktNumToSendAcks = nPktNum-nOffset;
			if ( nMinPktNumToSendAcks == m_receiverState.m_nMinPktNumToSendAcks )
				continue;
			if ( nMinPktNumToSendAcks < m_receiverState.m_nMinPktNumToSendAcks )
			{
				// Sender must never reduce this number!  Check for bugs or bogus sender
				if ( nPktNum >= m_receiverState.m_nPktNumUpdatedMinPktNumToSendAcks )
				{
					DECODE_ERROR( "SNP stop waiting reduced %lld (pkt %lld) -> %lld (pkt %lld)",
						(long long)m_receiverState.m_nMinPktNumToSendAcks,
						(long long)m_receiverState.m_nPktNumUpdatedMinPktNumToSendAcks,
						(long long)nMinPktNumToSendAcks,
						(long long)nPktNum
						);
				}
				continue;
			}
			SpewDebugGroup( nLogLevelPacketDecode, "[%s]   decode pkt %lld stop waiting: %lld (was %lld)",
				GetDescription(),
				(long long)nPktNum,
				(long long)nMinPktNumToSendAcks, (long long)m_receiverState.m_nMinPktNumToSendAcks );
			m_receiverState.m_nMinPktNumToSendAcks = nMinPktNumToSendAcks;
			m_receiverState.m_nPktNumUpdatedMinPktNumToSendAcks = nPktNum;

			// Trim from the front of the packet gap list,
			// we can stop reporting these losses to the sender
			auto h = m_receiverState.m_mapPacketGaps.begin();
			while ( h->first <= m_receiverState.m_nMinPktNumToSendAcks )
			{
				if ( h->second.m_nEnd > m_receiverState.m_nMinPktNumToSendAcks )
				{
					// Ug.  You're not supposed to modify the key in a map.
					// I suppose that's legit, since you could violate the ordering.
					// but in this case I know that this change is OK.
					const_cast<int64 &>( h->first ) = m_receiverState.m_nMinPktNumToSendAcks;
					break;
				}

				// Were we pending an ack on this?
				if ( m_receiverState.m_itPendingAck == h )
					++m_receiverState.m_itPendingAck;

				// Were we pending a nack on this?
				if ( m_receiverState.m_itPendingNack == h )
				{
					// I am not sure this is even possible.
					AssertMsg( false, "Expiring packet gap, which had pending NACK" );

					// But just in case, this would be the proper action
					++m_receiverState.m_itPendingNack;
				}

				// Packet loss is in the past.  Forget about it and move on
				h = m_receiverState.m_mapPacketGaps.erase(h);
			}
		}
		else if ( ( nFrameType & 0xf0 ) == 0x90 )
		{

			//
			// Ack
			//

			#if STEAMNETWORKINGSOCKETS_SNP_PARANOIA > 0
				m_senderState.DebugCheckInFlightPacketMap();
				#if STEAMNETWORKINGSOCKETS_SNP_PARANOIA == 1
				if ( ( nPktNum & 255 ) == 0 ) // only do it periodically
				#endif
				{
					m_senderState.DebugCheckInFlightPacketMap();
				}
			#endif

			// Parse latest received sequence number
			int64 nLatestRecvSeqNum;
			{
				static const char szAckLatestPktNum[] = "ack latest pktnum";
				int64 nLowerBits, nMask;
				if ( nFrameType & 0x40 )
				{
					READ_32BITU( nLowerBits, szAckLatestPktNum );
					nMask = 0xffffffff;
					nLatestRecvSeqNum = NearestWithSameLowerBits( (int32)nLowerBits, m_statsEndToEnd.m_nNextSendSequenceNumber );
				}
				else
				{
					READ_16BITU( nLowerBits, szAckLatestPktNum );
					nMask = 0xffff;
					nLatestRecvSeqNum = NearestWithSameLowerBits( (int16)nLowerBits, m_statsEndToEnd.m_nNextSendSequenceNumber );
				}
				Assert( ( nLatestRecvSeqNum & nMask ) == nLowerBits );

				// Find the message number that is closes to 
				if ( nLatestRecvSeqNum < 0 )
				{
					DECODE_ERROR( "SNP decode ack latest pktnum underflow.  %llx mod %llx, next send %llx",
						(unsigned long long)nLowerBits, (unsigned long long)( nMask+1 ), (unsigned long long)m_statsEndToEnd.m_nNextSendSequenceNumber );
				}
				if ( std::abs( nLatestRecvSeqNum - m_statsEndToEnd.m_nNextSendSequenceNumber ) > (nMask>>2) )
				{
					// We really should never get close to this boundary.
					SpewWarningRateLimited( usecNow, "Sender sent abs latest recv pkt number using %llx mod %llx, next send %llx\n",
						(unsigned long long)nLowerBits, (unsigned long long)( nMask+1 ), (unsigned long long)m_statsEndToEnd.m_nNextSendSequenceNumber );
				}
				if ( nLatestRecvSeqNum >= m_statsEndToEnd.m_nNextSendSequenceNumber )
				{
					DECODE_ERROR( "SNP decode ack latest pktnum %lld (%llx mod %llx), but next outoing packet is %lld (%llx).",
						(long long)nLatestRecvSeqNum, (unsigned long long)nLowerBits, (unsigned long long)( nMask+1 ),
						(long long)m_statsEndToEnd.m_nNextSendSequenceNumber, (unsigned long long)m_statsEndToEnd.m_nNextSendSequenceNumber
					);
				}
			}

			SpewDebugGroup( nLogLevelPacketDecode, "[%s]   decode pkt %lld latest recv %lld\n",
				GetDescription(),
				(long long)nPktNum, (long long)nLatestRecvSeqNum
			);

			// Locate our bookkeeping for this packet, or the latest one before it
			// Remember, we have a sentinel with a low, invalid packet number
			Assert( !m_senderState.m_mapInFlightPacketsByPktNum.empty() );
			auto inFlightPkt = m_senderState.m_mapInFlightPacketsByPktNum.upper_bound( nLatestRecvSeqNum );
			--inFlightPkt;
			Assert( inFlightPkt->first <= nLatestRecvSeqNum );

			// Parse out delay, and process the ping
			{
				uint16 nPackedDelay;
				READ_16BITU( nPackedDelay, "ack delay" );
				if ( nPackedDelay != 0xffff && inFlightPkt->first == nLatestRecvSeqNum && inFlightPkt->second.m_pTransport == ctx.m_pTransport )
				{
					SteamNetworkingMicroseconds usecDelay = SteamNetworkingMicroseconds( nPackedDelay ) << k_nAckDelayPrecisionShift;
					SteamNetworkingMicroseconds usecElapsed = usecNow - inFlightPkt->second.m_usecWhenSent;
					Assert( usecElapsed >= 0 );

					// Account for their reported delay, and calculate ping, in MS
					int msPing = ( usecElapsed - usecDelay ) / 1000;

					// Does this seem bogus?  (We allow a small amount of slop.)
					// NOTE: A malicious sender could lie about this delay, tricking us
					// into thinking that the real network latency is low, they are just
					// delaying their replies.  This actually matters, since the ping time
					// is an input into the rate calculation.  So we might need to
					// occasionally send pings that require an immediately reply, and
					// if those ping times seem way out of whack with the ones where they are
					// allowed to send a delay, take action against them.
					if ( msPing < -1 || msPing > 2000 )
					{
						// Either they are lying or some weird timer stuff is happening.
						// Either way, discard it.

						SpewMsgGroup( m_connectionConfig.m_LogLevel_AckRTT.Get(), "[%s] decode pkt %lld latest recv %lld delay %lluusec INVALID ping %lldusec\n",
							GetDescription(),
							(long long)nPktNum, (long long)nLatestRecvSeqNum,
							(unsigned long long)usecDelay,
							(long long)usecElapsed
						);
					}
					else
					{
						// Clamp, if we have slop
						if ( msPing < 0 )
							msPing = 0;
						ProcessSNPPing( msPing, ctx );

						// Spew
						SpewVerboseGroup( m_connectionConfig.m_LogLevel_AckRTT.Get(), "[%s] decode pkt %lld latest recv %lld delay %.1fms elapsed %.1fms ping %dms\n",
							GetDescription(),
							(long long)nPktNum, (long long)nLatestRecvSeqNum,
							(float)(usecDelay * 1e-3 ),
							(float)(usecElapsed * 1e-3 ),
							msPing
						);
					}
				}
			}

			// Parse number of blocks
			int nBlocks = nFrameType&7;
			if ( nBlocks == 7 )
				READ_8BITU( nBlocks, "ack num blocks" );

			// If they actually sent us any blocks, that means they are fragmented.
			// We should make sure and tell them to stop sending us these nacks
			// and move forward.
			if ( nBlocks > 0 )
			{
				// Decrease flush delay the more blocks they send us.
				// FIXME - This is not an optimal way to do this.  Forcing us to
				// ack everything is not what we want to do.  Instead, we should
				// use a separate timer for when we need to flush out a stop_waiting
				// packet!
				SteamNetworkingMicroseconds usecDelay = 250*1000 / nBlocks;
				QueueFlushAllAcks( usecNow + usecDelay );
			}

			// Process ack blocks, working backwards from the latest received sequence number.
			// Note that we have to parse all this stuff out, even if it's old news (packets older
			// than the stop_aiting value we sent), because we need to do that to get to the rest
			// of the packet.
			bool bAckedReliableRange = false;
			int64 nPktNumAckEnd = nLatestRecvSeqNum+1;
			while ( nBlocks >= 0 )
			{

				// Parse out number of acks/nacks.
				// Have we parsed all the real blocks?
				int64 nPktNumAckBegin, nPktNumNackBegin;
				if ( nBlocks == 0 )
				{
					// Implicit block.  Everything earlier between the last
					// NACK and the stop_waiting value is implicitly acked!
					if ( nPktNumAckEnd <= m_senderState.m_nMinPktWaitingOnAck )
						break;

					nPktNumAckBegin = m_senderState.m_nMinPktWaitingOnAck;
					nPktNumNackBegin = nPktNumAckBegin;
					SpewDebugGroup( nLogLevelPacketDecode, "[%s]   decode pkt %lld ack last block ack begin %lld\n",
						GetDescription(),
						(long long)nPktNum, (long long)nPktNumAckBegin );
				}
				else
				{
					uint8 nBlockHeader;
					READ_8BITU( nBlockHeader, "ack block header" );

					// Ack count?
					int64 numAcks = ( nBlockHeader>> 4 ) & 7;
					if ( nBlockHeader & 0x80 )
					{
						uint64 nUpperBits;
						READ_VARINT( nUpperBits, "ack count upper bits" );
						if ( nUpperBits > 100000 )
							DECODE_ERROR( "Ack count of %llu<<3 is crazy", (unsigned long long)nUpperBits );
						numAcks |= nUpperBits<<3;
					}
					nPktNumAckBegin = nPktNumAckEnd - numAcks;
					if ( nPktNumAckBegin < 0 )
						DECODE_ERROR( "Ack range underflow, end=%lld, num=%lld", (long long)nPktNumAckEnd, (long long)numAcks );

					// Extended nack count?
					int64 numNacks = nBlockHeader & 7;
					if ( nBlockHeader & 0x08)
					{
						uint64 nUpperBits;
						READ_VARINT( nUpperBits, "nack count upper bits" );
						if ( nUpperBits > 100000 )
							DECODE_ERROR( "Nack count of %llu<<3 is crazy", nUpperBits );
						numNacks |= nUpperBits<<3;
					}
					nPktNumNackBegin = nPktNumAckBegin - numNacks;
					if ( nPktNumNackBegin < 0 )
						DECODE_ERROR( "Nack range underflow, end=%lld, num=%lld", (long long)nPktNumAckBegin, (long long)numAcks );

					SpewDebugGroup( nLogLevelPacketDecode, "[%s]   decode pkt %lld nack [%lld,%lld) ack [%lld,%lld)\n",
						GetDescription(),
						(long long)nPktNum,
						(long long)nPktNumNackBegin, (long long)( nPktNumNackBegin + numNacks ),
						(long long)nPktNumAckBegin, (long long)( nPktNumAckBegin + numAcks )
					);
				}

				// Process acks first.
				Assert( nPktNumAckBegin >= 0 );
				while ( inFlightPkt->first >= nPktNumAckBegin )
				{
					Assert( inFlightPkt->first < nPktNumAckEnd );

					// Scan reliable segments, and see if any are marked for retry or are in flight
					for ( const SNPRange_t &relRange: inFlightPkt->second.m_vecReliableSegments )
					{

						// If range is present, it should be in only one of these two tables.
						if ( m_senderState.m_listInFlightReliableRange.erase( relRange ) == 0 )
						{
							if ( m_senderState.m_listReadyRetryReliableRange.erase( relRange ) > 0 )
							{

								// When we put stuff into the reliable retry list, we mark it as pending again.
								// But now it's acked, so it's no longer pending, even though we didn't send it.
								m_senderState.m_cbPendingReliable -= int( relRange.length() );
								Assert( m_senderState.m_cbPendingReliable >= 0 );

								bAckedReliableRange = true;
							}
						}
						else
						{
							bAckedReliableRange = true;
							Assert( m_senderState.m_listReadyRetryReliableRange.count( relRange ) == 0 );
						}
					}

					// Check if this was the next packet we were going to timeout, then advance
					// pointer.  This guy didn't timeout.
					if ( inFlightPkt == m_senderState.m_itNextInFlightPacketToTimeout )
						++m_senderState.m_itNextInFlightPacketToTimeout;

					// No need to track this anymore, remove from our table
					inFlightPkt = m_senderState.m_mapInFlightPacketsByPktNum.erase( inFlightPkt );
					--inFlightPkt;
					m_senderState.MaybeCheckInFlightPacketMap();
				}

				// Ack of in-flight end-to-end stats?
				if ( nPktNumAckBegin <= m_statsEndToEnd.m_pktNumInFlight && m_statsEndToEnd.m_pktNumInFlight < nPktNumAckEnd )
					m_statsEndToEnd.InFlightPktAck( usecNow );

				// Process nacks.
				Assert( nPktNumNackBegin >= 0 );
				while ( inFlightPkt->first >= nPktNumNackBegin )
				{
					Assert( inFlightPkt->first < nPktNumAckEnd );
					SNP_SenderProcessPacketNack( inFlightPkt->first, inFlightPkt->second, "NACK" );

					// We'll keep the record on hand, though, in case an ACK comes in
					--inFlightPkt;
				}

				// Continue on to the the next older block
				nPktNumAckEnd = nPktNumNackBegin;
				--nBlocks;
			}

			// Should we check for discarding reliable messages we are keeping around in case
			// of retransmission, since we know now that they were delivered?
			if ( bAckedReliableRange )
			{
				m_senderState.RemoveAckedReliableMessageFromUnackedList();

				// Spew where we think the peer is decoding the reliable stream
				if ( nLogLevelPacketDecode >= k_ESteamNetworkingSocketsDebugOutputType_Debug )
				{

					int64 nPeerReliablePos = m_senderState.m_nReliableStreamPos;
					if ( !m_senderState.m_listInFlightReliableRange.empty() )
						nPeerReliablePos = std::min( nPeerReliablePos, m_senderState.m_listInFlightReliableRange.begin()->first.m_nBegin );
					if ( !m_senderState.m_listReadyRetryReliableRange.empty() )
						nPeerReliablePos = std::min( nPeerReliablePos, m_senderState.m_listReadyRetryReliableRange.begin()->first.m_nBegin );

					SpewDebugGroup( nLogLevelPacketDecode, "[%s]   decode pkt %lld peer reliable pos = %lld\n",
						GetDescription(),
						(long long)nPktNum, (long long)nPeerReliablePos );
				}
			}

			// Check if any of this was new info, then advance our stop_waiting value.
			if ( nLatestRecvSeqNum > m_senderState.m_nMinPktWaitingOnAck )
			{
				SpewVerboseGroup( nLogLevelPacketDecode, "[%s]   updating min_waiting_on_ack %lld -> %lld\n",
					GetDescription(),
					(long long)m_senderState.m_nMinPktWaitingOnAck, (long long)nLatestRecvSeqNum );
				m_senderState.m_nMinPktWaitingOnAck = nLatestRecvSeqNum;
			}
		}
		else
		{
			DECODE_ERROR( "Invalid SNP frame lead byte 0x%02x", nFrameType );
		}
	}

	// Should we record that we received it?
	if ( bInhibitMarkReceived )
	{
		// Something really odd.  High packet loss / fragmentation.
		// Potentially the peer is being abusive and we need
		// to protect ourselves.
		//
		// Act as if the packet was dropped.  This will cause the
		// peer's sender logic to interpret this as additional packet
		// loss and back off.  That's a feature, not a bug.
	}
	else
	{

		// Update structures needed to populate our ACKs.
		// If we received reliable data now, then schedule an ack
		bool bScheduleAck = nDecodeReliablePos > 0;
		SNP_RecordReceivedPktNum( nPktNum, usecNow, bScheduleAck );
	}

	// Track end-to-end flow.  Even if we decided to tell our peer that
	// we did not receive this, we want our own stats to reflect
	// that we did.  (And we want to be able to quickly reject a
	// packet with this same number.)
	//
	// Also, note that order of operations is important.  This call must
	// happen after the SNP_RecordReceivedPktNum call above
	m_statsEndToEnd.TrackProcessSequencedPacket( nPktNum, usecNow, usecTimeSinceLast );

	// Packet can be processed further
	return true;

	// Make sure these don't get used beyond where we intended them to get used
	#undef DECODE_ERROR
	#undef EXPECT_BYTES
	#undef READ_8BITU
	#undef READ_16BITU
	#undef READ_24BITU
	#undef READ_32BITU
	#undef READ_64BITU
	#undef READ_VARINT
	#undef READ_SEGMENT_DATA_SIZE
}