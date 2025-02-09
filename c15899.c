static int decode_trns_chunk(AVCodecContext *avctx, PNGDecContext *s,
                             uint32_t length)
{
    int v, i;

    if (s->color_type == PNG_COLOR_TYPE_PALETTE) {
        if (length > 256 || !(s->state & PNG_PLTE))
            return AVERROR_INVALIDDATA;

        for (i = 0; i < length; i++) {
            v = bytestream2_get_byte(&s->gb);
            s->palette[i] = (s->palette[i] & 0x00ffffff) | (v << 24);
        }
    } else if (s->color_type == PNG_COLOR_TYPE_GRAY || s->color_type == PNG_COLOR_TYPE_RGB) {
        if ((s->color_type == PNG_COLOR_TYPE_GRAY && length != 2) ||
            (s->color_type == PNG_COLOR_TYPE_RGB && length != 6))
            return AVERROR_INVALIDDATA;

        for (i = 0; i < length / 2; i++) {
            /* only use the least significant bits */
            v = av_mod_uintp2(bytestream2_get_be16(&s->gb), s->bit_depth);

            if (s->bit_depth > 8)
                AV_WB16(&s->transparent_color_be[2 * i], v);
            else
                s->transparent_color_be[i] = v;
        }
    } else {
        return AVERROR_INVALIDDATA;
    }

    bytestream2_skip(&s->gb, 4); /* crc */
    s->has_trns = 1;

    return 0;
}