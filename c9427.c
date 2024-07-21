HttpHeaderEntry::parse(const char *field_start, const char *field_end, const http_hdr_owner_type msgType)
{
    /* note: name_start == field_start */
    const char *name_end = (const char *)memchr(field_start, ':', field_end - field_start);
    int name_len = name_end ? name_end - field_start :0;
    const char *value_start = field_start + name_len + 1;   /* skip ':' */
    /* note: value_end == field_end */

    ++ HeaderEntryParsedCount;

    /* do we have a valid field name within this field? */

    if (!name_len || name_end > field_end)
        return NULL;

    if (name_len > 65534) {
        /* String must be LESS THAN 64K and it adds a terminating NULL */
        // TODO: update this to show proper name_len in Raw markup, but not print all that
        debugs(55, 2, "ignoring huge header field (" << Raw("field_start", field_start, 100) << "...)");
        return NULL;
    }

    /*
     * RFC 7230 section 3.2.4:
     * "No whitespace is allowed between the header field-name and colon.
     * ...
     *  A server MUST reject any received request message that contains
     *  whitespace between a header field-name and colon with a response code
     *  of 400 (Bad Request).  A proxy MUST remove any such whitespace from a
     *  response message before forwarding the message downstream."
     */
    if (xisspace(field_start[name_len - 1])) {

        if (msgType == hoRequest)
            return nullptr;

        // for now, also let relaxed parser remove this BWS from any non-HTTP messages
        const bool stripWhitespace = (msgType == hoReply) ||
                                     Config.onoff.relaxed_header_parser;
        if (!stripWhitespace)
            return nullptr; // reject if we cannot strip

        debugs(55, Config.onoff.relaxed_header_parser <= 0 ? 1 : 2,
               "NOTICE: Whitespace after header name in '" << getStringPrefix(field_start, field_end-field_start) << "'");

        while (name_len > 0 && xisspace(field_start[name_len - 1]))
            --name_len;

        if (!name_len) {
            debugs(55, 2, "found header with only whitespace for name");
            return NULL;
        }
    }

    /* now we know we can parse it */

    debugs(55, 9, "parsing HttpHeaderEntry: near '" <<  getStringPrefix(field_start, field_end-field_start) << "'");

    /* is it a "known" field? */
    Http::HdrType id = Http::HeaderLookupTable.lookup(field_start,name_len).id;
    debugs(55, 9, "got hdr-id=" << id);

    String name;

    String value;

    if (id == Http::HdrType::BAD_HDR)
        id = Http::HdrType::OTHER;

    /* set field name */
    if (id == Http::HdrType::OTHER)
        name.limitInit(field_start, name_len);
    else
        name = Http::HeaderLookupTable.lookup(id).name;

    /* trim field value */
    while (value_start < field_end && xisspace(*value_start))
        ++value_start;

    while (value_start < field_end && xisspace(field_end[-1]))
        --field_end;

    if (field_end - value_start > 65534) {
        /* String must be LESS THAN 64K and it adds a terminating NULL */
        debugs(55, 2, "WARNING: found '" << name << "' header of " << (field_end - value_start) << " bytes");
        return NULL;
    }

    /* set field value */
    value.limitInit(value_start, field_end - value_start);

    if (id != Http::HdrType::BAD_HDR)
        ++ headerStatsTable[id].seenCount;

    debugs(55, 9, "parsed HttpHeaderEntry: '" << name << ": " << value << "'");

    return new HttpHeaderEntry(id, name.termedBuf(), value.termedBuf());
}