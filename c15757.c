static int filter_frame(AVFilterLink *inlink, AVFrame *frame)
{
    AVFilterContext   *ctx     = inlink->dst;
    FieldOrderContext *s       = ctx->priv;
    AVFilterLink      *outlink = ctx->outputs[0];
    int h, plane, line_step, line_size, line;
    uint8_t *data;

    if (!frame->interlaced_frame ||
        frame->top_field_first == s->dst_tff)
        return ff_filter_frame(outlink, frame);

    av_dlog(ctx,
            "picture will move %s one line\n",
            s->dst_tff ? "up" : "down");
    h = frame->height;
    for (plane = 0; plane < 4 && frame->data[plane]; plane++) {
        line_step = frame->linesize[plane];
        line_size = s->line_size[plane];
        data = frame->data[plane];
        if (s->dst_tff) {
            /** Move every line up one line, working from
             *  the top to the bottom of the frame.
             *  The original top line is lost.
             *  The new last line is created as a copy of the
             *  penultimate line from that field. */
            for (line = 0; line < h; line++) {
                if (1 + line < frame->height) {
                    memcpy(data, data + line_step, line_size);
                } else {
                    memcpy(data, data - line_step - line_step, line_size);
                }
                data += line_step;
            }
        } else {
            /** Move every line down one line, working from
             *  the bottom to the top of the frame.
             *  The original bottom line is lost.
             *  The new first line is created as a copy of the
             *  second line from that field. */
            data += (h - 1) * line_step;
            for (line = h - 1; line >= 0 ; line--) {
                if (line > 0) {
                    memcpy(data, data - line_step, line_size);
                } else {
                    memcpy(data, data + line_step + line_step, line_size);
                }
                data -= line_step;
            }
        }
    }
    frame->top_field_first = s->dst_tff;

    return ff_filter_frame(outlink, frame);
}