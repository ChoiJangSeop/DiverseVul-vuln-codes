static int handle_eac3(MOVMuxContext *mov, AVPacket *pkt, MOVTrack *track)
{
    GetBitContext gbc;
    AC3HeaderInfo tmp, *hdr = &tmp;
    struct eac3_info *info;
    int num_blocks;

    if (!track->eac3_priv && !(track->eac3_priv = av_mallocz(sizeof(*info))))
        return AVERROR(ENOMEM);
    info = track->eac3_priv;

    init_get_bits(&gbc, pkt->data, pkt->size * 8);
    if (avpriv_ac3_parse_header(&gbc, &hdr) < 0) {
        /* drop the packets until we see a good one */
        if (!track->entry) {
            av_log(mov, AV_LOG_WARNING, "Dropping invalid packet from start of the stream\n");
            return 0;
        }
        return AVERROR_INVALIDDATA;
    }

    info->data_rate = FFMAX(info->data_rate, hdr->bit_rate / 1000);
    num_blocks = hdr->num_blocks;

    if (!info->ec3_done) {
        /* AC-3 substream must be the first one */
        if (hdr->bitstream_id <= 10 && hdr->substreamid != 0)
            return AVERROR(EINVAL);

        /* this should always be the case, given that our AC-3 parser
         * concatenates dependent frames to their independent parent */
        if (hdr->frame_type == EAC3_FRAME_TYPE_INDEPENDENT) {
            /* substream ids must be incremental */
            if (hdr->substreamid > info->num_ind_sub + 1)
                return AVERROR(EINVAL);

            if (hdr->substreamid == info->num_ind_sub + 1) {
                //info->num_ind_sub++;
                avpriv_request_sample(track->par, "Multiple independent substreams");
                return AVERROR_PATCHWELCOME;
            } else if (hdr->substreamid < info->num_ind_sub ||
                       hdr->substreamid == 0 && info->substream[0].bsid) {
                info->ec3_done = 1;
                goto concatenate;
            }
        }

        /* fill the info needed for the "dec3" atom */
        info->substream[hdr->substreamid].fscod = hdr->sr_code;
        info->substream[hdr->substreamid].bsid  = hdr->bitstream_id;
        info->substream[hdr->substreamid].bsmod = hdr->bitstream_mode;
        info->substream[hdr->substreamid].acmod = hdr->channel_mode;
        info->substream[hdr->substreamid].lfeon = hdr->lfe_on;

        /* Parse dependent substream(s), if any */
        if (pkt->size != hdr->frame_size) {
            int cumul_size = hdr->frame_size;
            int parent = hdr->substreamid;

            while (cumul_size != pkt->size) {
                int i;
                init_get_bits(&gbc, pkt->data + cumul_size, (pkt->size - cumul_size) * 8);
                if (avpriv_ac3_parse_header(&gbc, &hdr) < 0)
                    return AVERROR_INVALIDDATA;
                if (hdr->frame_type != EAC3_FRAME_TYPE_DEPENDENT)
                    return AVERROR(EINVAL);
                cumul_size += hdr->frame_size;
                info->substream[parent].num_dep_sub++;

                /* header is parsed up to lfeon, but custom channel map may be needed */
                /* skip bsid */
                skip_bits(&gbc, 5);
                /* skip volume control params */
                for (i = 0; i < (hdr->channel_mode ? 1 : 2); i++) {
                    skip_bits(&gbc, 5); // skip dialog normalization
                    if (get_bits1(&gbc)) {
                        skip_bits(&gbc, 8); // skip compression gain word
                    }
                }
                /* get the dependent stream channel map, if exists */
                if (get_bits1(&gbc))
                    info->substream[parent].chan_loc |= (get_bits(&gbc, 16) >> 5) & 0x1f;
                else
                    info->substream[parent].chan_loc |= hdr->channel_mode;
            }
        }
    }

concatenate:
    if (!info->num_blocks && num_blocks == 6)
        return pkt->size;
    else if (info->num_blocks + num_blocks > 6)
        return AVERROR_INVALIDDATA;

    if (!info->num_blocks) {
        int ret;
        if ((ret = av_copy_packet(&info->pkt, pkt)) < 0)
            return ret;
        info->num_blocks = num_blocks;
        return 0;
    } else {
        int ret;
        if ((ret = av_grow_packet(&info->pkt, pkt->size)) < 0)
            return ret;
        memcpy(info->pkt.data + info->pkt.size - pkt->size, pkt->data, pkt->size);
        info->num_blocks += num_blocks;
        info->pkt.duration += pkt->duration;
        if ((ret = av_copy_packet_side_data(&info->pkt, pkt)) < 0)
            return ret;
        if (info->num_blocks != 6)
            return 0;
        av_packet_unref(pkt);
        if ((ret = av_copy_packet(pkt, &info->pkt)) < 0)
            return ret;
        av_packet_unref(&info->pkt);
        info->num_blocks = 0;
    }

    return pkt->size;
}