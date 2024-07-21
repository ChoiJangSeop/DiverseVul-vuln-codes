xps_true_callback_glyph_name(gs_font *pfont, gs_glyph glyph, gs_const_string *pstr)
{
    /* This function is copied verbatim from plfont.c */

    int table_length;
    int table_offset;

    ulong format;
    uint numGlyphs;
    uint glyph_name_index;
    const byte *postp; /* post table pointer */

    /* guess if the font type is not truetype */
    if ( pfont->FontType != ft_TrueType )
    {
        glyph -= 29;
        if (glyph < 258 )
        {
            pstr->data = (byte*) pl_mac_names[glyph];
            pstr->size = strlen((char*)pstr->data);
            return 0;
        }
        else
        {
            return gs_throw1(-1, "glyph index %lu out of range", (ulong)glyph);
        }
    }

    table_offset = xps_find_sfnt_table((xps_font_t*)pfont->client_data, "post", &table_length);

    /* no post table */
    if (table_offset < 0)
        return gs_throw(-1, "no post table");

    /* this shoudn't happen but... */
    if ( table_length == 0 )
        return gs_throw(-1, "zero-size post table");

    ((gs_font_type42 *)pfont)->data.string_proc((gs_font_type42 *)pfont,
                                                table_offset, table_length, &postp);
    format = u32(postp);

    /* Format 1.0 (mac encoding) is a simple table see the TT spec.
     * We don't implement this because we don't see it in practice.
     * Format 2.5 is deprecated.
     * Format 3.0 means that there is no post data in the font file.
     * We see this a lot but can't do much about it.
     * The only format we support is 2.0.
     */
    if ( format != 0x20000 )
    {
        /* Invent a name if we don't know the table format. */
        char buf[32];
        gs_sprintf(buf, "glyph%d", (int)glyph);
        pstr->data = (byte*)buf;
        pstr->size = strlen((char*)pstr->data);
        return 0;
    }

    /* skip over the post header */
    numGlyphs = u16(postp + 32);
    if (glyph > numGlyphs - 1)
    {
        return gs_throw1(-1, "glyph index %lu out of range", (ulong)glyph);
    }

    /* glyph name index starts at post + 34 each entry is 2 bytes */
    glyph_name_index = u16(postp + 34 + (glyph * 2));

    /* this shouldn't happen */
    if ( glyph_name_index > 0x7fff )
        return gs_throw(-1, "post table format error");

    /* mac easy */
    if ( glyph_name_index < 258 )
    {
        // dmprintf2(pfont->memory, "glyph name (mac) %d = %s\n", glyph, pl_mac_names[glyph_name_index]);
        pstr->data = (byte*) pl_mac_names[glyph_name_index];
        pstr->size = strlen((char*)pstr->data);
        return 0;
    }

    /* not mac */
    else
    {
        byte *mydata;

        /* and here's the tricky part */
        const byte *pascal_stringp = postp + 34 + (numGlyphs * 2);

        /* 0 - 257 lives in the mac table above */
        glyph_name_index -= 258;

        /* The string we want is the index'th pascal string,
         * so we "hop" to each length byte "index" times. */
        while (glyph_name_index > 0)
        {
            pascal_stringp += ((int)(*pascal_stringp)+1);
            glyph_name_index--;
        }

        /* length byte */
        pstr->size = (int)(*pascal_stringp);

        /* + 1 is for the length byte */
        pstr->data = pascal_stringp + 1;

        /* sanity check */
        if ( pstr->data + pstr->size > postp + table_length || pstr->data - 1 < postp)
            return gs_throw(-1, "data out of range");

        /* sigh - we have to allocate a copy of the data - by the
         * time a high level device makes use of it the font data
         * may be freed. This is a necessary leak. */
        mydata = gs_alloc_bytes(pfont->memory, pstr->size + 1, "glyph to name");
        if ( mydata == 0 )
            return -1;
        memcpy(mydata, pascal_stringp + 1, pstr->size);
        pstr->data = mydata;

        mydata[pstr->size] = 0;

        return 0;
    }
}