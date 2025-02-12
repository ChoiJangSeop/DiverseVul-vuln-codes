static int filter_frame(AVFilterLink *inlink, AVFrame *in)
{
    AVFilterContext *ctx  = inlink->dst;
    FlipContext *s     = ctx->priv;
    AVFilterLink *outlink = ctx->outputs[0];
    AVFrame *out;
    uint8_t *inrow, *outrow;
    int i, j, plane, step;

    out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
    if (!out) {
        av_frame_free(&in);
        return AVERROR(ENOMEM);
    }
    av_frame_copy_props(out, in);

    /* copy palette if required */
    if (av_pix_fmt_desc_get(inlink->format)->flags & AV_PIX_FMT_FLAG_PAL)
        memcpy(out->data[1], in->data[1], AVPALETTE_SIZE);

    for (plane = 0; plane < 4 && in->data[plane]; plane++) {
        const int width  = (plane == 1 || plane == 2) ? FF_CEIL_RSHIFT(inlink->w, s->hsub) : inlink->w;
        const int height = (plane == 1 || plane == 2) ? FF_CEIL_RSHIFT(inlink->h, s->vsub) : inlink->h;
        step = s->max_step[plane];

        outrow = out->data[plane];
        inrow  = in ->data[plane] + (width - 1) * step;
        for (i = 0; i < height; i++) {
            switch (step) {
            case 1:
                for (j = 0; j < width; j++)
                    outrow[j] = inrow[-j];
            break;

            case 2:
            {
                uint16_t *outrow16 = (uint16_t *)outrow;
                uint16_t * inrow16 = (uint16_t *) inrow;
                for (j = 0; j < width; j++)
                    outrow16[j] = inrow16[-j];
            }
            break;

            case 3:
            {
                uint8_t *in  =  inrow;
                uint8_t *out = outrow;
                for (j = 0; j < width; j++, out += 3, in -= 3) {
                    int32_t v = AV_RB24(in);
                    AV_WB24(out, v);
                }
            }
            break;

            case 4:
            {
                uint32_t *outrow32 = (uint32_t *)outrow;
                uint32_t * inrow32 = (uint32_t *) inrow;
                for (j = 0; j < width; j++)
                    outrow32[j] = inrow32[-j];
            }
            break;

            default:
                for (j = 0; j < width; j++)
                    memcpy(outrow + j*step, inrow - j*step, step);
            }

            inrow  += in ->linesize[plane];
            outrow += out->linesize[plane];
        }
    }

    av_frame_free(&in);
    return ff_filter_frame(outlink, out);
}