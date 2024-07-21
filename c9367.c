static int get_unicode(textw_text_enum_t *penum, gs_font *font, gs_glyph glyph, gs_char ch, unsigned short *Buffer)
{
    int code;
    gs_const_string gnstr;
    unsigned short fallback = ch;
    ushort *unicode = NULL;
    int length;

    length = font->procs.decode_glyph((gs_font *)font, glyph, ch, NULL, 0);
    if (length == 0) {
        code = font->procs.glyph_name(font, glyph, &gnstr);
        if (code >= 0 && gnstr.size == 7) {
            if (!memcmp(gnstr.data, "uni", 3)) {
                static const char *hexdigits = "0123456789ABCDEF";
                char *d0 = strchr(hexdigits, gnstr.data[3]);
                char *d1 = strchr(hexdigits, gnstr.data[4]);
                char *d2 = strchr(hexdigits, gnstr.data[5]);
                char *d3 = strchr(hexdigits, gnstr.data[6]);

                if (d0 != NULL && d1 != NULL && d2 != NULL && d3 != NULL) {
                    *Buffer++ = ((d0 - hexdigits) << 12) + ((d1 - hexdigits) << 8) + ((d2 - hexdigits) << 4) + (d3 - hexdigits);
                    return 1;
                }
            }
        }
        if (length == 0) {
            single_glyph_list_t *sentry = (single_glyph_list_t *)&SingleGlyphList;
            double_glyph_list_t *dentry = (double_glyph_list_t *)&DoubleGlyphList;
            treble_glyph_list_t *tentry = (treble_glyph_list_t *)&TrebleGlyphList;
            quad_glyph_list_t *qentry = (quad_glyph_list_t *)&QuadGlyphList;

            /* Search glyph to single Unicode value table */
            while (sentry->Glyph != 0) {
                if (sentry->Glyph[0] < gnstr.data[0]) {
                    sentry++;
                    continue;
                }
                if (sentry->Glyph[0] > gnstr.data[0]){
                    break;
                }
                if (strlen(sentry->Glyph) == gnstr.size) {
                    if(memcmp(gnstr.data, sentry->Glyph, gnstr.size) == 0) {
                        *Buffer = sentry->Unicode;
                        return 1;
                    }
                }
                sentry++;
            }

            /* Search glyph to double Unicode value table */
            while (dentry->Glyph != 0) {
                if (dentry->Glyph[0] < gnstr.data[0]) {
                    dentry++;
                    continue;
                }
                if (dentry->Glyph[0] > gnstr.data[0]){
                    break;
                }
                if (strlen(dentry->Glyph) == gnstr.size) {
                    if(memcmp(gnstr.data, dentry->Glyph, gnstr.size) == 0) {
                        memcpy(Buffer, dentry->Unicode, 2);
                        return 2;
                    }
                }
                dentry++;
            }

            /* Search glyph to triple Unicode value table */
            while (tentry->Glyph != 0) {
                if (tentry->Glyph[0] < gnstr.data[0]) {
                    tentry++;
                    continue;
                }
                if (tentry->Glyph[0] > gnstr.data[0]){
                    break;
                }
                if (strlen(tentry->Glyph) == gnstr.size) {
                    if(memcmp(gnstr.data, tentry->Glyph, gnstr.size) == 0) {
                        memcpy(Buffer, tentry->Unicode, 3);
                        return 3;
                    }
                }
                tentry++;
            }

            /* Search glyph to quadruple Unicode value table */
            while (qentry->Glyph != 0) {
                if (qentry->Glyph[0] < gnstr.data[0]) {
                    qentry++;
                    continue;
                }
                if (qentry->Glyph[0] > gnstr.data[0]){
                    break;
                }
                if (strlen(qentry->Glyph) == gnstr.size) {
                    if(memcmp(gnstr.data, qentry->Glyph, gnstr.size) == 0) {
                        memcpy(Buffer, qentry->Unicode, 4);
                        return 4;
                    }
                }
                qentry++;
            }
        }
        *Buffer = fallback;
        return 1;
    } else {
        char *b, *u;
        int l = length - 1;

        unicode = (ushort *)gs_alloc_bytes(penum->dev->memory, length, "temporary Unicode array");
        length = font->procs.decode_glyph((gs_font *)font, glyph, ch, unicode, length);
#if ARCH_IS_BIG_ENDIAN
        memcpy(Buffer, unicode, length);
#else
        b = (char *)Buffer;
        u = (char *)unicode;
        while (l >= 0) {
            *b++ = *(u + l);
            l--;
        }

#endif
        gs_free_object(penum->dev->memory, unicode, "free temporary unicode buffer");
        return length / sizeof(short);
    }
}