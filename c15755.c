static int filter_frame(AVFilterLink *inlink, AVFrame *in)
{
    PadContext *s = inlink->dst->priv;
    AVFrame *out;
    int needs_copy = frame_needs_copy(s, in);

    if (needs_copy) {
        av_log(inlink->dst, AV_LOG_DEBUG, "Direct padding impossible allocating new frame\n");
        out = ff_get_video_buffer(inlink->dst->outputs[0],
                                  FFMAX(inlink->w, s->w),
                                  FFMAX(inlink->h, s->h));
        if (!out) {
            av_frame_free(&in);
            return AVERROR(ENOMEM);
        }

        av_frame_copy_props(out, in);
    } else {
        int i;

        out = in;
        for (i = 0; i < 4 && out->data[i]; i++) {
            int hsub = s->draw.hsub[i];
            int vsub = s->draw.vsub[i];
            out->data[i] -= (s->x >> hsub) * s->draw.pixelstep[i] +
                            (s->y >> vsub) * out->linesize[i];
        }
    }

    /* top bar */
    if (s->y) {
        ff_fill_rectangle(&s->draw, &s->color,
                          out->data, out->linesize,
                          0, 0, s->w, s->y);
    }

    /* bottom bar */
    if (s->h > s->y + s->in_h) {
        ff_fill_rectangle(&s->draw, &s->color,
                          out->data, out->linesize,
                          0, s->y + s->in_h, s->w, s->h - s->y - s->in_h);
    }

    /* left border */
    ff_fill_rectangle(&s->draw, &s->color, out->data, out->linesize,
                      0, s->y, s->x, in->height);

    if (needs_copy) {
        ff_copy_rectangle2(&s->draw,
                          out->data, out->linesize, in->data, in->linesize,
                          s->x, s->y, 0, 0, in->width, in->height);
    }

    /* right border */
    ff_fill_rectangle(&s->draw, &s->color, out->data, out->linesize,
                      s->x + s->in_w, s->y, s->w - s->x - s->in_w,
                      in->height);

    out->width  = s->w;
    out->height = s->h;

    if (in != out)
        av_frame_free(&in);
    return ff_filter_frame(inlink->dst->outputs[0], out);
}