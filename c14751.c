static int shorten_decode_frame(AVCodecContext *avctx, void *data,
                                int *got_frame_ptr, AVPacket *avpkt)
{
    AVFrame *frame     = data;
    const uint8_t *buf = avpkt->data;
    int buf_size       = avpkt->size;
    ShortenContext *s  = avctx->priv_data;
    int i, input_buf_size = 0;
    int ret;

    /* allocate internal bitstream buffer */
    if (s->max_framesize == 0) {
        void *tmp_ptr;
        s->max_framesize = 1024; // should hopefully be enough for the first header
        tmp_ptr = av_fast_realloc(s->bitstream, &s->allocated_bitstream_size,
                                  s->max_framesize);
        if (!tmp_ptr) {
            av_log(avctx, AV_LOG_ERROR, "error allocating bitstream buffer\n");
            return AVERROR(ENOMEM);
        }
        s->bitstream = tmp_ptr;
    }

    /* append current packet data to bitstream buffer */
    if (1 && s->max_framesize) { //FIXME truncated
        buf_size       = FFMIN(buf_size, s->max_framesize - s->bitstream_size);
        input_buf_size = buf_size;

        if (s->bitstream_index + s->bitstream_size + buf_size >
            s->allocated_bitstream_size) {
            memmove(s->bitstream, &s->bitstream[s->bitstream_index],
                    s->bitstream_size);
            s->bitstream_index = 0;
        }
        if (buf)
            memcpy(&s->bitstream[s->bitstream_index + s->bitstream_size], buf,
                   buf_size);
        buf               = &s->bitstream[s->bitstream_index];
        buf_size         += s->bitstream_size;
        s->bitstream_size = buf_size;

        /* do not decode until buffer has at least max_framesize bytes or
         * the end of the file has been reached */
        if (buf_size < s->max_framesize && avpkt->data) {
            *got_frame_ptr = 0;
            return input_buf_size;
        }
    }
    /* init and position bitstream reader */
    init_get_bits(&s->gb, buf, buf_size * 8);
    skip_bits(&s->gb, s->bitindex);

    /* process header or next subblock */
    if (!s->got_header) {
        if ((ret = read_header(s)) < 0)
            return ret;
        *got_frame_ptr = 0;
        goto finish_frame;
    }

    /* if quit command was read previously, don't decode anything */
    if (s->got_quit_command) {
        *got_frame_ptr = 0;
        return avpkt->size;
    }

    s->cur_chan = 0;
    while (s->cur_chan < s->channels) {
        unsigned cmd;
        int len;

        if (get_bits_left(&s->gb) < 3 + FNSIZE) {
            *got_frame_ptr = 0;
            break;
        }

        cmd = get_ur_golomb_shorten(&s->gb, FNSIZE);

        if (cmd > FN_VERBATIM) {
            av_log(avctx, AV_LOG_ERROR, "unknown shorten function %d\n", cmd);
            *got_frame_ptr = 0;
            break;
        }

        if (!is_audio_command[cmd]) {
            /* process non-audio command */
            switch (cmd) {
            case FN_VERBATIM:
                len = get_ur_golomb_shorten(&s->gb, VERBATIM_CKSIZE_SIZE);
                while (len--)
                    get_ur_golomb_shorten(&s->gb, VERBATIM_BYTE_SIZE);
                break;
            case FN_BITSHIFT:
                s->bitshift = get_ur_golomb_shorten(&s->gb, BITSHIFTSIZE);
                break;
            case FN_BLOCKSIZE: {
                unsigned blocksize = get_uint(s, av_log2(s->blocksize));
                if (blocksize > s->blocksize) {
                    av_log(avctx, AV_LOG_ERROR,
                           "Increasing block size is not supported\n");
                    return AVERROR_PATCHWELCOME;
                }
                if (!blocksize || blocksize > MAX_BLOCKSIZE) {
                    av_log(avctx, AV_LOG_ERROR, "invalid or unsupported "
                                                "block size: %d\n", blocksize);
                    return AVERROR(EINVAL);
                }
                s->blocksize = blocksize;
                break;
            }
            case FN_QUIT:
                s->got_quit_command = 1;
                break;
            }
            if (cmd == FN_BLOCKSIZE || cmd == FN_QUIT) {
                *got_frame_ptr = 0;
                break;
            }
        } else {
            /* process audio command */
            int residual_size = 0;
            int channel = s->cur_chan;
            int32_t coffset;

            /* get Rice code for residual decoding */
            if (cmd != FN_ZERO) {
                residual_size = get_ur_golomb_shorten(&s->gb, ENERGYSIZE);
                /* This is a hack as version 0 differed in the definition
                 * of get_sr_golomb_shorten(). */
                if (s->version == 0)
                    residual_size--;
            }

            /* calculate sample offset using means from previous blocks */
            if (s->nmean == 0)
                coffset = s->offset[channel][0];
            else {
                int32_t sum = (s->version < 2) ? 0 : s->nmean / 2;
                for (i = 0; i < s->nmean; i++)
                    sum += s->offset[channel][i];
                coffset = sum / s->nmean;
                if (s->version >= 2)
                    coffset >>= FFMIN(1, s->bitshift);
            }

            /* decode samples for this channel */
            if (cmd == FN_ZERO) {
                for (i = 0; i < s->blocksize; i++)
                    s->decoded[channel][i] = 0;
            } else {
                if ((ret = decode_subframe_lpc(s, cmd, channel,
                                               residual_size, coffset)) < 0)
                    return ret;
            }

            /* update means with info from the current block */
            if (s->nmean > 0) {
                int32_t sum = (s->version < 2) ? 0 : s->blocksize / 2;
                for (i = 0; i < s->blocksize; i++)
                    sum += s->decoded[channel][i];

                for (i = 1; i < s->nmean; i++)
                    s->offset[channel][i - 1] = s->offset[channel][i];

                if (s->version < 2)
                    s->offset[channel][s->nmean - 1] = sum / s->blocksize;
                else
                    s->offset[channel][s->nmean - 1] = (sum / s->blocksize) << s->bitshift;
            }

            /* copy wrap samples for use with next block */
            for (i = -s->nwrap; i < 0; i++)
                s->decoded[channel][i] = s->decoded[channel][i + s->blocksize];

            /* shift samples to add in unused zero bits which were removed
             * during encoding */
            fix_bitshift(s, s->decoded[channel]);

            /* if this is the last channel in the block, output the samples */
            s->cur_chan++;
            if (s->cur_chan == s->channels) {
                /* get output buffer */
                frame->nb_samples = s->blocksize;
                if ((ret = ff_get_buffer(avctx, frame, 0)) < 0) {
                    av_log(avctx, AV_LOG_ERROR, "get_buffer() failed\n");
                    return ret;
                }
                /* interleave output */
                output_buffer((int16_t **)frame->extended_data, s->channels,
                              s->blocksize, s->decoded);

                *got_frame_ptr = 1;
            }
        }
    }
    if (s->cur_chan < s->channels)
        *got_frame_ptr = 0;

finish_frame:
    s->bitindex = get_bits_count(&s->gb) - 8 * (get_bits_count(&s->gb) / 8);
    i           = get_bits_count(&s->gb) / 8;
    if (i > buf_size) {
        av_log(s->avctx, AV_LOG_ERROR, "overread: %d\n", i - buf_size);
        s->bitstream_size  = 0;
        s->bitstream_index = 0;
        return AVERROR_INVALIDDATA;
    }
    if (s->bitstream_size) {
        s->bitstream_index += i;
        s->bitstream_size  -= i;
        return input_buf_size;
    } else
        return i;
}