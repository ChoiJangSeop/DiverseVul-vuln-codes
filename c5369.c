xps_encode_font_char_imp(xps_font_t *font, int code)
{
    byte *table;

    /* no cmap selected: return identity */
    if (font->cmapsubtable <= 0)
        return code;

    table = font->data + font->cmapsubtable;

    switch (u16(table))
    {
    case 0: /* Apple standard 1-to-1 mapping. */
        return table[code + 6];

    case 4: /* Microsoft/Adobe segmented mapping. */
        {
            int segCount2 = u16(table + 6);
            byte *endCount = table + 14;
            byte *startCount = endCount + segCount2 + 2;
            byte *idDelta = startCount + segCount2;
            byte *idRangeOffset = idDelta + segCount2;
            int i2;

            for (i2 = 0; i2 < segCount2 - 3; i2 += 2)
            {
                int delta, roff;
                int start = u16(startCount + i2);
                int glyph;

                if ( code < start )
                    return 0;
                if ( code > u16(endCount + i2) )
                    continue;
                delta = s16(idDelta + i2);
                roff = s16(idRangeOffset + i2);
                if ( roff == 0 )
                {
                    return ( code + delta ) & 0xffff; /* mod 65536 */
                    return 0;
                }
                glyph = u16(idRangeOffset + i2 + roff + ((code - start) << 1));
                return (glyph == 0 ? 0 : glyph + delta);
            }

            /*
             * The TrueType documentation says that the last range is
             * always supposed to end with 0xffff, so this shouldn't
             * happen; however, in some real fonts, it does.
             */
            return 0;
        }

    case 6: /* Single interval lookup. */
        {
            int firstCode = u16(table + 6);
            int entryCount = u16(table + 8);
            if ( code < firstCode || code >= firstCode + entryCount )
                return 0;
            return u16(table + 10 + ((code - firstCode) << 1));
        }

    case 10: /* Trimmed array (like 6) */
        {
            int startCharCode = u32(table + 12);
            int numChars = u32(table + 16);
            if ( code < startCharCode || code >= startCharCode + numChars )
                return 0;
            return u32(table + 20 + (code - startCharCode) * 4);
        }

    case 12: /* Segmented coverage. (like 4) */
        {
            int nGroups = u32(table + 12);
            byte *group = table + 16;
            int i;

            for (i = 0; i < nGroups; i++)
            {
                int startCharCode = u32(group + 0);
                int endCharCode = u32(group + 4);
                int startGlyphID = u32(group + 8);
                if ( code < startCharCode )
                    return 0;
                if ( code <= endCharCode )
                    return startGlyphID + (code - startCharCode);
                group += 12;
            }

            return 0;
        }

    case 2: /* High-byte mapping through table. */
    case 8: /* Mixed 16-bit and 32-bit coverage (like 2) */
    default:
        gs_warn1("unknown cmap format: %d\n", u16(table));
        return 0;
    }

    return 0;
}