
static int read_gab2_sub(AVFormatContext *s, AVStream *st, AVPacket *pkt)
{
    if (pkt->size >= 7 &&
        pkt->size < INT_MAX - AVPROBE_PADDING_SIZE &&
        !strcmp(pkt->data, "GAB2") && AV_RL16(pkt->data + 5) == 2) {
        uint8_t desc[256];
        int score      = AVPROBE_SCORE_EXTENSION, ret;
        AVIStream *ast = st->priv_data;
        AVInputFormat *sub_demuxer;
        AVRational time_base;
        int size;
        AVIOContext *pb = avio_alloc_context(pkt->data + 7,
                                             pkt->size - 7,
                                             0, NULL, NULL, NULL, NULL);
        AVProbeData pd;
        unsigned int desc_len = avio_rl32(pb);

        if (desc_len > pb->buf_end - pb->buf_ptr)
            goto error;

        ret = avio_get_str16le(pb, desc_len, desc, sizeof(desc));
        avio_skip(pb, desc_len - ret);
        if (*desc)
            av_dict_set(&st->metadata, "title", desc, 0);

        avio_rl16(pb);   /* flags? */
        avio_rl32(pb);   /* data size */

        size = pb->buf_end - pb->buf_ptr;
        pd = (AVProbeData) { .buf      = av_mallocz(size + AVPROBE_PADDING_SIZE),
                             .buf_size = size };
        if (!pd.buf)
            goto error;
        memcpy(pd.buf, pb->buf_ptr, size);
        sub_demuxer = av_probe_input_format2(&pd, 1, &score);
        av_freep(&pd.buf);
        if (!sub_demuxer)
            goto error;

        if (!(ast->sub_ctx = avformat_alloc_context()))
            goto error;

        ast->sub_ctx->pb = pb;

        if (ff_copy_whiteblacklists(ast->sub_ctx, s) < 0)
            goto error;

        if (!avformat_open_input(&ast->sub_ctx, "", sub_demuxer, NULL)) {
            if (ast->sub_ctx->nb_streams != 1)
                goto error;
            ff_read_packet(ast->sub_ctx, &ast->sub_pkt);
            avcodec_parameters_copy(st->codecpar, ast->sub_ctx->streams[0]->codecpar);
            time_base = ast->sub_ctx->streams[0]->time_base;
            avpriv_set_pts_info(st, 64, time_base.num, time_base.den);
        }
        ast->sub_buffer = pkt->data;
        memset(pkt, 0, sizeof(*pkt));
        return 1;

error:
        av_freep(&ast->sub_ctx);
        av_freep(&pb);
    }
    return 0;