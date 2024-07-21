GF_Err gp_rtp_builder_do_avc(GP_RTPPacketizer *builder, u8 *nalu, u32 nalu_size, u8 IsAUEnd, u32 FullAUSize)
{
	u32 do_flush, bytesLeft, size, nal_type;
	char shdr[2];
	char stap_hdr;

	do_flush = 0;
	if (!nalu) do_flush = 1;
	/*we only do STAP or SINGLE modes*/
	else if (builder->sl_header.accessUnitStartFlag) do_flush = 1;
	/*we must NOT fragment a NALU*/
	else if (builder->bytesInPacket + nalu_size >= builder->Path_MTU) do_flush = 2;
	/*aggregation is disabled*/
	else if (! (builder->flags & GP_RTP_PCK_USE_MULTI) ) do_flush = 2;

	if (builder->bytesInPacket && do_flush) {
		builder->rtp_header.Marker = (do_flush==1) ? 1 : 0;
		builder->OnPacketDone(builder->cbk_obj, &builder->rtp_header);
		builder->bytesInPacket = 0;
	}

	if (!nalu) return GF_OK;

	/*need a new RTP packet*/
	if (!builder->bytesInPacket) {
		builder->rtp_header.PayloadType = builder->PayloadType;
		builder->rtp_header.Marker = 0;
		builder->rtp_header.TimeStamp = (u32) builder->sl_header.compositionTimeStamp;
		builder->rtp_header.SequenceNumber += 1;
		builder->OnNewPacket(builder->cbk_obj, &builder->rtp_header);
		builder->avc_non_idr = GF_TRUE;
	}

	/*check NAL type to see if disposable or not*/
	nal_type = nalu[0] & 0x1F;
	switch (nal_type) {
	case GF_AVC_NALU_NON_IDR_SLICE:
	case GF_AVC_NALU_ACCESS_UNIT:
	case GF_AVC_NALU_END_OF_SEQ:
	case GF_AVC_NALU_END_OF_STREAM:
	case GF_AVC_NALU_FILLER_DATA:
		break;
	default:
		builder->avc_non_idr = GF_FALSE;
		break;
	}

	/*at this point we're sure the NALU fits in current packet OR must be splitted*/

	/*pb: we don't know if next NALU from this AU will be small enough to fit in the packet, so we always
	go for stap...*/
	if (builder->bytesInPacket+nalu_size<builder->Path_MTU) {
		Bool use_stap = GF_TRUE;
		/*if this is the AU end and no NALU in packet, go for single mode*/
		if (IsAUEnd && !builder->bytesInPacket) use_stap = GF_FALSE;

		if (use_stap) {
			/*declare STAP-A NAL*/
			if (!builder->bytesInPacket) {
				/*copy over F and NRI from first nal in packet and assign type*/
				stap_hdr = (nalu[0] & 0xE0) | 24;
				builder->OnData(builder->cbk_obj, (char *) &stap_hdr, 1, GF_FALSE);
				builder->bytesInPacket = 1;
			}
			/*add NALU size*/
			shdr[0] = nalu_size>>8;
			shdr[1] = nalu_size&0x00ff;
			builder->OnData(builder->cbk_obj, (char *)shdr, 2, GF_FALSE);
			builder->bytesInPacket += 2;
		}
		/*add data*/
		if (builder->OnDataReference)
			builder->OnDataReference(builder->cbk_obj, nalu_size, 0);
		else
			builder->OnData(builder->cbk_obj, nalu, nalu_size, GF_FALSE);

		builder->bytesInPacket += nalu_size;

		if (IsAUEnd) {
			builder->rtp_header.Marker = 1;
			builder->OnPacketDone(builder->cbk_obj, &builder->rtp_header);
			builder->bytesInPacket = 0;
		}
	}
	/*fragmentation unit*/
	else {
		u32 offset;
		assert(nalu_size>=builder->Path_MTU);
		assert(!builder->bytesInPacket);
		/*FU payload doesn't have the NAL hdr*/
		bytesLeft = nalu_size - 1;
		offset = 1;
		while (bytesLeft) {
			if (2 + bytesLeft > builder->Path_MTU) {
				size = builder->Path_MTU - 2;
			} else {
				size = bytesLeft;
			}

			/*copy over F and NRI from nal in packet and assign type*/
			shdr[0] = (nalu[0] & 0xE0) | 28;
			/*copy over NAL type from nal and set start bit and end bit*/
			shdr[1] = (nalu[0] & 0x1F);
			/*start bit*/
			if (offset==1) shdr[1] |= 0x80;
			/*end bit*/
			else if (size == bytesLeft) shdr[1] |= 0x40;

			builder->OnData(builder->cbk_obj, (char *)shdr, 2, GF_FALSE);

			/*add data*/
			if (builder->OnDataReference)
				builder->OnDataReference(builder->cbk_obj, size, offset);
			else
				builder->OnData(builder->cbk_obj, nalu+offset, size, GF_FALSE);

			offset += size;
			bytesLeft -= size;

			/*flush no matter what (FUs cannot be agreggated)*/
			builder->rtp_header.Marker = (IsAUEnd && !bytesLeft) ? 1 : 0;
			builder->OnPacketDone(builder->cbk_obj, &builder->rtp_header);
			builder->bytesInPacket = 0;

			if (bytesLeft) {
				builder->rtp_header.PayloadType = builder->PayloadType;
				builder->rtp_header.TimeStamp = (u32) builder->sl_header.compositionTimeStamp;
				builder->rtp_header.SequenceNumber += 1;
				builder->OnNewPacket(builder->cbk_obj, &builder->rtp_header);
			}
		}
	}

	return GF_OK;
}