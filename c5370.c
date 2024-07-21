xps_decode_font_char_imp(xps_font_t *font, int code)
{
    byte *table;

    /* no cmap selected: return identity */
    if (font->cmapsubtable <= 0)
        return code;

    table = font->data + font->cmapsubtable;

    switch (u16(table))
    {
        case 0: /* Apple standard 1-to-1 mapping. */
            {
                int i, length = u16(&table[2]) - 6;

                if (length < 0 || length > 256)
                    return gs_error_invalidfont;

                for (i=0;i<length;i++) {
                    if (table[6 + i] == code)
                        return i;
                }
            }
            return 0;
        case 4: /* Microsoft/Adobe segmented mapping. */
            {
                int segCount2 = u16(table + 6);
                byte *endCount = table + 14;
                byte *startCount = endCount + segCount2 + 2;
                byte *idDelta = startCount + segCount2;
                byte *idRangeOffset = idDelta + segCount2;
                int i2;

                if (segCount2 < 3 || segCount2 > 65535)
                    return gs_error_invalidfont;

                for (i2 = 0; i2 < segCount2 - 3; i2 += 2)
                {
                    int delta = s16(idDelta + i2), roff = s16(idRangeOffset + i2);
                    int start = u16(startCount + i2);
                    int end = u16(endCount + i2);
                    int glyph, i;

                    if (end < start)
                        return gs_error_invalidfont;

                    for (i=start;i<=end;i++) {
                        if (roff == 0) {
                            glyph = (i + delta) & 0xffff;
                        } else {
                            glyph = u16(idRangeOffset + i2 + roff + ((i - start) << 1));
                        }
                        if (glyph == code) {
                            return i;
                        }
                    }
                }
            }
            return 0;
        case 6: /* Single interval lookup. */
            {
                int ch, i, length = u16(&table[8]);
                int firstCode = u16(&table[6]);

                if (length < 0 || length > 65535)
                    return gs_error_invalidfont;

                for (i=0;i<length;i++) {
                    ch = u16(&table[10 + (i * 2)]);
                    if (ch == code)
                        return (firstCode + i);
                }
            }
            return 0;
        case 10: /* Trimmed array (like 6) */
            {
                unsigned int ch, i, length = u32(&table[20]);
                int firstCode = u32(&table[16]);
                for (i=0;i<length;i++) {
                    ch = u16(&table[10 + (i * 2)]);
                    if (ch == code)
                        return (firstCode + i);
                }
            }
            return 0;
        case 12: /* Segmented coverage. (like 4) */
            {
                unsigned int nGroups = u32(&table[12]);
                int Group;

                for (Group=0;Group<nGroups;Group++)
                {
                    int startCharCode = u32(&table[16 + (Group * 12)]);
                    int endCharCode = u32(&table[16 + (Group * 12) + 4]);
                    int startGlyphCode = u32(&table[16 + (Group * 12) + 8]);

                    if (code >= startGlyphCode && code <= (startGlyphCode + (endCharCode - startCharCode))) {
                        return startGlyphCode + (code - startCharCode);
                    }
                }
            }
            return 0;
        case 2: /* High-byte mapping through table. */
        case 8: /* Mixed 16-bit and 32-bit coverage (like 2) */
        default:
            gs_warn1("unknown cmap format: %d\n", u16(table));
            return 0;
    }
    return 0;
}