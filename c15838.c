njs_string_trim(const njs_value_t *value, njs_string_prop_t *string,
    unsigned mode)
{
    uint32_t              cp, trim;
    const u_char          *p, *prev, *start, *end;
    njs_unicode_decode_t  ctx;

    trim = 0;

    njs_string_prop(string, value);

    start = string->start;
    end = string->start + string->size;

    if (njs_is_byte_or_ascii_string(string)) {

        if (mode & NJS_TRIM_START) {
            for ( ;; ) {
                if (start == end) {
                    break;
                }

                if (njs_is_whitespace(*start)) {
                    start++;
                    trim++;
                    continue;
                }

                break;
            }
        }

        if (mode & NJS_TRIM_END) {
            for ( ;; ) {
                if (start == end) {
                    break;
                }

                end--;

                if (njs_is_whitespace(*end)) {
                    trim++;
                    continue;
                }

                end++;
                break;
            }
        }

    } else {
        /* UTF-8 string. */

        if (mode & NJS_TRIM_START) {
            njs_utf8_decode_init(&ctx);

            for ( ;; ) {
                if (start == end) {
                    break;
                }

                p = start;
                cp = njs_utf8_decode(&ctx, &start, end);

                if (njs_utf8_is_whitespace(cp)) {
                    trim++;
                    continue;
                }

                start = p;
                break;
            }
        }

        if (mode & NJS_TRIM_END) {
            prev = end;

            njs_utf8_decode_init(&ctx);

            for ( ;; ) {
                if (start == prev) {
                    break;
                }

                prev = njs_utf8_prev(prev);
                p = prev;
                cp = njs_utf8_decode(&ctx, &p, end);

                if (njs_utf8_is_whitespace(cp)) {
                    trim++;
                    continue;
                }

                end = p;
                break;
            }
        }
    }

    if (start == end) {
        string->length = 0;
        string->size = 0;
        return trim;
    }

    string->start = (u_char *) start;
    string->size = end - start;

    if (string->length != 0) {
        string->length -= trim;
    }

    return trim;
}