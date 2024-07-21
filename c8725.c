static void ZRLE_ENCODE_TILE(VncState *vs, ZRLE_PIXEL *data, int w, int h,
                             int zywrle_level)
{
    VncPalette *palette = &vs->zrle.palette;

    int runs = 0;
    int single_pixels = 0;

    bool use_rle;
    bool use_palette;

    int i;

    ZRLE_PIXEL *ptr = data;
    ZRLE_PIXEL *end = ptr + h * w;
    *end = ~*(end-1); /* one past the end is different so the while loop ends */

    /* Real limit is 127 but we wan't a way to know if there is more than 127 */
    palette_init(palette, 256, ZRLE_BPP);

    while (ptr < end) {
        ZRLE_PIXEL pix = *ptr;
        if (*++ptr != pix) { /* FIXME */
            single_pixels++;
        } else {
            while (*++ptr == pix) ;
            runs++;
        }
        palette_put(palette, pix);
    }

    /* Solid tile is a special case */

    if (palette_size(palette) == 1) {
        bool found;

        vnc_write_u8(vs, 1);
        ZRLE_WRITE_PIXEL(vs, palette_color(palette, 0, &found));
        return;
    }

    zrle_choose_palette_rle(vs, w, h, palette, ZRLE_BPP_OUT,
                            runs, single_pixels, zywrle_level,
                            &use_rle, &use_palette);

    if (!use_palette) {
        vnc_write_u8(vs, (use_rle ? 128 : 0));
    } else {
        uint32_t colors[VNC_PALETTE_MAX_SIZE];
        size_t size = palette_size(palette);

        vnc_write_u8(vs, (use_rle ? 128 : 0) | size);
        palette_fill(palette, colors);

        for (i = 0; i < size; i++) {
            ZRLE_WRITE_PIXEL(vs, colors[i]);
        }
    }

    if (use_rle) {
        ZRLE_PIXEL *ptr = data;
        ZRLE_PIXEL *end = ptr + w * h;
        ZRLE_PIXEL *run_start;
        ZRLE_PIXEL pix;

        while (ptr < end) {
            int len;
            int index = 0;

            run_start = ptr;
            pix = *ptr++;

            while (*ptr == pix && ptr < end) {
                ptr++;
            }

            len = ptr - run_start;

            if (use_palette)
                index = palette_idx(palette, pix);

            if (len <= 2 && use_palette) {
                if (len == 2) {
                    vnc_write_u8(vs, index);
                }
                vnc_write_u8(vs, index);
                continue;
            }
            if (use_palette) {
                vnc_write_u8(vs, index | 128);
            } else {
                ZRLE_WRITE_PIXEL(vs, pix);
            }

            len -= 1;

            while (len >= 255) {
                vnc_write_u8(vs, 255);
                len -= 255;
            }

            vnc_write_u8(vs, len);
        }
    } else if (use_palette) { /* no RLE */
        int bppp;
        ZRLE_PIXEL *ptr = data;

        /* packed pixels */

        assert (palette_size(palette) < 17);

        bppp = bits_per_packed_pixel[palette_size(palette)-1];

        for (i = 0; i < h; i++) {
            uint8_t nbits = 0;
            uint8_t byte = 0;

            ZRLE_PIXEL *eol = ptr + w;

            while (ptr < eol) {
                ZRLE_PIXEL pix = *ptr++;
                uint8_t index = palette_idx(palette, pix);

                byte = (byte << bppp) | index;
                nbits += bppp;
                if (nbits >= 8) {
                    vnc_write_u8(vs, byte);
                    nbits = 0;
                }
            }
            if (nbits > 0) {
                byte <<= 8 - nbits;
                vnc_write_u8(vs, byte);
            }
        }
    } else {

        /* raw */

#if ZRLE_BPP != 8
        if (zywrle_level > 0 && !(zywrle_level & 0x80)) {
            ZYWRLE_ANALYZE(data, data, w, h, w, zywrle_level, vs->zywrle.buf);
            ZRLE_ENCODE_TILE(vs, data, w, h, zywrle_level | 0x80);
        }
        else
#endif
        {
#ifdef ZRLE_COMPACT_PIXEL
            ZRLE_PIXEL *ptr;

            for (ptr = data; ptr < data + w * h; ptr++) {
                ZRLE_WRITE_PIXEL(vs, *ptr);
            }
#else
            vnc_write(vs, data, w * h * (ZRLE_BPP / 8));
#endif
        }
    }
}