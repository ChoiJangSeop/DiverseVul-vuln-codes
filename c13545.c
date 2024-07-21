GF_Err gp_rtp_builder_do_hevc(GP_RTPPacketizer *builder, u8 *nalu, u32 nalu_size, u8 IsAUEnd, u32 FullAUSize)
{
	u32 do_flush, bytesLeft, size;

	do_flush = 0;
	if (!nalu) do_flush = 1;
	else if (builder->sl_header.accessUnitStartFlag) do_flush = 1;
	/*we must NOT fragment a NALU*/
	else if (builder->bytesInPacket + nalu_size + 4 >= builder->Path_MTU) do_flush = 2; //2 bytes PayloadHdr for AP + 2 bytes NAL size
	/*aggregation is disabled*/
	else if (! (builder->flags & GP_RTP_PCK_USE_MULTI) ) do_flush = 2;

	if (builder->bytesInPacket && do_flush) {
		builder->rtp_header.Marker = (do_flush==1) ? 1 : 0;
		/*insert payload_hdr in case of AP*/
		if (strlen(builder->hevc_payload_hdr)) {
			builder->OnData(builder->cbk_obj, (char *)builder->hevc_payload_hdr, 2, GF_TRUE);
			memset(builder->hevc_payload_hdr, 0, 2);
		}
		builder->OnPacketDone(builder->cbk_obj, &builder->rtp_header);
		builder->bytesInPacket = 0;
	}

	if (!nalu) return GF_OK;

	/*need a new RTP packet*/
	if (!builder->bytesInPacket) {
		builder->rtp_header.PayloadType = builder->PayloadType;
		builder->rtp_header.TimeStamp = (u32) builder->sl_header.compositionTimeStamp;
		builder->rtp_header.SequenceNumber += 1;
		builder->OnNewPacket(builder->cbk_obj, &builder->rtp_header);
	}

	/*at this point we're sure the NALU fits in current packet OR must be splitted*/
	/*check that we should use single NALU packet mode or aggreation packets mode*/
	if (builder->bytesInPacket+nalu_size+4 < builder->Path_MTU) {
		Bool use_AP = (builder->flags & GP_RTP_PCK_USE_MULTI) ? GF_TRUE : GF_FALSE;
		/*if this is the AU end and no NALU in packet, go for single NALU packet mode*/
		if (IsAUEnd && !builder->bytesInPacket) use_AP = GF_FALSE;

		if (use_AP) {
			char nal_s[2];
			/*declare PayloadHdr for AP*/
			if (!builder->bytesInPacket) {
				/*copy F bit and assign type*/
				builder->hevc_payload_hdr[0] = (nalu[0] & 0x81) | (48 << 1);
				/*copy LayerId and TID*/
				builder->hevc_payload_hdr[1] = nalu[1];
			}
			else {
				/*F bit of AP is 0 if the F nit of each aggreated NALU is 0; otherwise its must be 1*/
				/*LayerId and TID must ne the lowest value of LayerId and TID of all aggreated NALU*/
				u8 cur_LayerId, cur_TID, new_LayerId, new_TID;

				builder->hevc_payload_hdr[0] |= (nalu[0] & 0x80);
				cur_LayerId = ((builder->hevc_payload_hdr[0] & 0x01) << 5) + ((builder->hevc_payload_hdr[1] & 0xF8) >> 3);
				new_LayerId = ((nalu[0] & 0x01) << 5) + ((nalu[1] & 0xF8) >> 3);
				if (cur_LayerId > new_LayerId) {
					builder->hevc_payload_hdr[0] = (builder->hevc_payload_hdr[0] & 0xFE) | (nalu[0] & 0x01);
					builder->hevc_payload_hdr[1] = (builder->hevc_payload_hdr[1] & 0x07) | (nalu[1] & 0xF8);
				}
				cur_TID = builder->hevc_payload_hdr[1] & 0x07;
				new_TID = nalu[1] & 0x07;
				if (cur_TID > new_TID) {
					builder->hevc_payload_hdr[1] = (builder->hevc_payload_hdr[1] & 0xF8) | (nalu[1] & 0x07);
				}
			}

			/*add NALU size*/
			nal_s[0] = nalu_size>>8;
			nal_s[1] = nalu_size&0x00ff;
			builder->OnData(builder->cbk_obj, (char *)nal_s, 2, GF_FALSE);
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
			if (strlen(builder->hevc_payload_hdr)) {
				builder->OnData(builder->cbk_obj, (char *)builder->hevc_payload_hdr, 2, GF_TRUE);
				memset(builder->hevc_payload_hdr, 0, 2);
			}
			builder->OnPacketDone(builder->cbk_obj, &builder->rtp_header);
			builder->bytesInPacket = 0;
		}
	}
	/*fragmentation unit*/
	else {
		u32 offset;
		char payload_hdr[2];
		char shdr;

		assert(nalu_size + 4 >=builder->Path_MTU);
		assert(!builder->bytesInPacket);

		/*FU payload doesn't have the NAL hdr (2 bytes*/
		bytesLeft = nalu_size - 2;
		offset = 2;
		while (bytesLeft) {
			if (3 + bytesLeft > builder->Path_MTU) {
				size = builder->Path_MTU - 3;
			} else {
				size = bytesLeft;
			}

			/*declare PayloadHdr for FU*/
			memset(payload_hdr, 0, 2);
			/*copy F bit and assign type*/
			payload_hdr[0] = (nalu[0] & 0x81) | (49 << 1);
			/*copy LayerId and TID*/
			payload_hdr[1] = nalu[1];
			builder->OnData(builder->cbk_obj, (char *)payload_hdr, 2, GF_FALSE);

			/*declare FU header*/
			shdr = 0;
			/*assign type*/
			shdr |= (nalu[0] & 0x7E) >> 1;
			/*start bit*/
			if (offset==2) shdr |= 0x80;
			/*end bit*/
			else if (size == bytesLeft) shdr |= 0x40;

			builder->OnData(builder->cbk_obj, &shdr, 1, GF_FALSE);

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