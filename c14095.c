static int vorbis_header(AVFormatContext *s, int idx)
{
    struct ogg *ogg = s->priv_data;
    AVStream *st    = s->streams[idx];
    struct ogg_stream *os = ogg->streams + idx;
    struct oggvorbis_private *priv;
    int pkt_type = os->buf[os->pstart];

    if (!os->private) {
        os->private = av_mallocz(sizeof(struct oggvorbis_private));
        if (!os->private)
            return AVERROR(ENOMEM);
    }

    if (!(pkt_type & 1))
        return 0;

    if (os->psize < 1 || pkt_type > 5)
        return AVERROR_INVALIDDATA;

    priv = os->private;

    if (priv->packet[pkt_type >> 1])
        return AVERROR_INVALIDDATA;
    if (pkt_type > 1 && !priv->packet[0] || pkt_type > 3 && !priv->packet[1])
        return AVERROR_INVALIDDATA;

    priv->len[pkt_type >> 1]    = os->psize;
    priv->packet[pkt_type >> 1] = av_mallocz(os->psize);
    if (!priv->packet[pkt_type >> 1])
        return AVERROR(ENOMEM);
    memcpy(priv->packet[pkt_type >> 1], os->buf + os->pstart, os->psize);
    if (os->buf[os->pstart] == 1) {
        const uint8_t *p = os->buf + os->pstart + 7; /* skip "\001vorbis" tag */
        unsigned blocksize, bs0, bs1;
        int srate;
        int channels;

        if (os->psize != 30)
            return AVERROR_INVALIDDATA;

        if (bytestream_get_le32(&p) != 0) /* vorbis_version */
            return AVERROR_INVALIDDATA;

        channels = bytestream_get_byte(&p);
        if (st->codec->channels && channels != st->codec->channels) {
            av_log(s, AV_LOG_ERROR, "Channel change is not supported\n");
            return AVERROR_PATCHWELCOME;
        }
        st->codec->channels = channels;
        srate               = bytestream_get_le32(&p);
        p += 4; // skip maximum bitrate
        st->codec->bit_rate = bytestream_get_le32(&p); // nominal bitrate
        p += 4; // skip minimum bitrate

        blocksize = bytestream_get_byte(&p);
        bs0       = blocksize & 15;
        bs1       = blocksize >> 4;

        if (bs0 > bs1)
            return AVERROR_INVALIDDATA;
        if (bs0 < 6 || bs1 > 13)
            return AVERROR_INVALIDDATA;

        if (bytestream_get_byte(&p) != 1) /* framing_flag */
            return AVERROR_INVALIDDATA;

        st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
        st->codec->codec_id   = AV_CODEC_ID_VORBIS;

        if (srate > 0) {
            st->codec->sample_rate = srate;
            avpriv_set_pts_info(st, 64, 1, srate);
        }
    } else if (os->buf[os->pstart] == 3) {
        if (vorbis_update_metadata(s, idx) >= 0) {
            // drop all metadata we parsed and which is not required by libvorbis
            unsigned new_len = 7 + 4 + AV_RL32(priv->packet[1] + 7) + 4 + 1;
            if (new_len >= 16 && new_len < os->psize) {
                AV_WL32(priv->packet[1] + new_len - 5, 0);
                priv->packet[1][new_len - 1] = 1;
                priv->len[1]                 = new_len;
            }
        }
    } else {
        int ret = fixup_vorbis_headers(s, priv, &st->codec->extradata);
        if (ret < 0) {
            st->codec->extradata_size = 0;
            return ret;
        }
        st->codec->extradata_size = ret;
        if ((ret = avpriv_vorbis_parse_extradata(st->codec, &priv->vp))) {
            av_freep(&st->codec->extradata);
            st->codec->extradata_size = 0;
            return ret;
        }
    }

    return 1;
}