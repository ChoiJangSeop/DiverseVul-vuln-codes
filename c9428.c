HttpHeader::parse(const char *header_start, size_t hdrLen)
{
    const char *field_ptr = header_start;
    const char *header_end = header_start + hdrLen; // XXX: remove
    int warnOnError = (Config.onoff.relaxed_header_parser <= 0 ? DBG_IMPORTANT : 2);

    PROF_start(HttpHeaderParse);

    assert(header_start && header_end);
    debugs(55, 7, "parsing hdr: (" << this << ")" << std::endl << getStringPrefix(header_start, hdrLen));
    ++ HttpHeaderStats[owner].parsedCount;

    char *nulpos;
    if ((nulpos = (char*)memchr(header_start, '\0', hdrLen))) {
        debugs(55, DBG_IMPORTANT, "WARNING: HTTP header contains NULL characters {" <<
               getStringPrefix(header_start, nulpos-header_start) << "}\nNULL\n{" << getStringPrefix(nulpos+1, hdrLen-(nulpos-header_start)-1));
        PROF_stop(HttpHeaderParse);
        clean();
        return 0;
    }

    Http::ContentLengthInterpreter clen(warnOnError);
    /* common format headers are "<name>:[ws]<value>" lines delimited by <CRLF>.
     * continuation lines start with a (single) space or tab */
    while (field_ptr < header_end) {
        const char *field_start = field_ptr;
        const char *field_end;

        do {
            const char *this_line = field_ptr;
            field_ptr = (const char *)memchr(field_ptr, '\n', header_end - field_ptr);

            if (!field_ptr) {
                // missing <LF>
                PROF_stop(HttpHeaderParse);
                clean();
                return 0;
            }

            field_end = field_ptr;

            ++field_ptr;    /* Move to next line */

            if (field_end > this_line && field_end[-1] == '\r') {
                --field_end;    /* Ignore CR LF */

                if (owner == hoRequest && field_end > this_line) {
                    bool cr_only = true;
                    for (const char *p = this_line; p < field_end && cr_only; ++p) {
                        if (*p != '\r')
                            cr_only = false;
                    }
                    if (cr_only) {
                        debugs(55, DBG_IMPORTANT, "SECURITY WARNING: Rejecting HTTP request with a CR+ "
                               "header field to prevent request smuggling attacks: {" <<
                               getStringPrefix(header_start, hdrLen) << "}");
                        PROF_stop(HttpHeaderParse);
                        clean();
                        return 0;
                    }
                }
            }

            /* Barf on stray CR characters */
            if (memchr(this_line, '\r', field_end - this_line)) {
                debugs(55, warnOnError, "WARNING: suspicious CR characters in HTTP header {" <<
                       getStringPrefix(field_start, field_end-field_start) << "}");

                if (Config.onoff.relaxed_header_parser) {
                    char *p = (char *) this_line;   /* XXX Warning! This destroys original header content and violates specifications somewhat */

                    while ((p = (char *)memchr(p, '\r', field_end - p)) != NULL) {
                        *p = ' ';
                        ++p;
                    }
                } else {
                    PROF_stop(HttpHeaderParse);
                    clean();
                    return 0;
                }
            }

            if (this_line + 1 == field_end && this_line > field_start) {
                debugs(55, warnOnError, "WARNING: Blank continuation line in HTTP header {" <<
                       getStringPrefix(header_start, hdrLen) << "}");
                PROF_stop(HttpHeaderParse);
                clean();
                return 0;
            }
        } while (field_ptr < header_end && (*field_ptr == ' ' || *field_ptr == '\t'));

        if (field_start == field_end) {
            if (field_ptr < header_end) {
                debugs(55, warnOnError, "WARNING: unparseable HTTP header field near {" <<
                       getStringPrefix(field_start, hdrLen-(field_start-header_start)) << "}");
                PROF_stop(HttpHeaderParse);
                clean();
                return 0;
            }

            break;      /* terminating blank line */
        }

        const auto e = HttpHeaderEntry::parse(field_start, field_end, owner);
        if (!e) {
            debugs(55, warnOnError, "WARNING: unparseable HTTP header field {" <<
                   getStringPrefix(field_start, field_end-field_start) << "}");
            debugs(55, warnOnError, " in {" << getStringPrefix(header_start, hdrLen) << "}");

            PROF_stop(HttpHeaderParse);
            clean();
            return 0;
        }

        if (e->id == Http::HdrType::CONTENT_LENGTH && !clen.checkField(e->value)) {
            delete e;

            if (Config.onoff.relaxed_header_parser)
                continue; // clen has printed any necessary warnings

            PROF_stop(HttpHeaderParse);
            clean();
            return 0;
        }

        if (e->id == Http::HdrType::OTHER && stringHasWhitespace(e->name.termedBuf())) {
            debugs(55, warnOnError, "WARNING: found whitespace in HTTP header name {" <<
                   getStringPrefix(field_start, field_end-field_start) << "}");

            if (!Config.onoff.relaxed_header_parser) {
                delete e;
                PROF_stop(HttpHeaderParse);
                clean();
                return 0;
            }
        }

        addEntry(e);
    }

    if (clen.headerWideProblem) {
        debugs(55, warnOnError, "WARNING: " << clen.headerWideProblem <<
               " Content-Length field values in" <<
               Raw("header", header_start, hdrLen));
    }

    if (chunked()) {
        // RFC 2616 section 4.4: ignore Content-Length with Transfer-Encoding
        // RFC 7230 section 3.3.3 #3: Transfer-Encoding overwrites Content-Length
        delById(Http::HdrType::CONTENT_LENGTH);
        // and clen state becomes irrelevant
    } else if (clen.sawBad) {
        // ensure our callers do not accidentally see bad Content-Length values
        delById(Http::HdrType::CONTENT_LENGTH);
        conflictingContentLength_ = true; // TODO: Rename to badContentLength_.
    } else if (clen.needsSanitizing) {
        // RFC 7230 section 3.3.2: MUST either reject or ... [sanitize];
        // ensure our callers see a clean Content-Length value or none at all
        delById(Http::HdrType::CONTENT_LENGTH);
        if (clen.sawGood) {
            putInt64(Http::HdrType::CONTENT_LENGTH, clen.value);
            debugs(55, 5, "sanitized Content-Length to be " << clen.value);
        }
    }

    PROF_stop(HttpHeaderParse);
    return 1;           /* even if no fields where found, it is a valid header */
}