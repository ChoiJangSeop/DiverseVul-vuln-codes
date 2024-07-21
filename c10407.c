httpHdrContRangeParseInit(HttpHdrContRange * range, const char *str)
{
    const char *p;
    assert(range && str);
    debugs(68, 8, "parsing content-range field: '" << str << "'");
    /* check range type */

    if (strncasecmp(str, "bytes ", 6))
        return 0;

    str += 6;

    /* split */
    if (!(p = strchr(str, '/')))
        return 0;

    if (*str == '*')
        range->spec.offset = range->spec.length = range_spec_unknown;
    else if (!httpHdrRangeRespSpecParseInit(&range->spec, str, p - str))
        return 0;

    ++p;

    if (*p == '*')
        range->elength = range_spec_unknown;
    else if (!httpHeaderParseOffset(p, &range->elength))
        return 0;
    else if (range->elength <= 0) {
        /* Additional paranoidal check for BUG2155 - entity-length MUST be > 0 */
        debugs(68, 2, "invalid (entity-length is negative) content-range-spec near: '" << str << "'");
        return 0;
    } else if (known_spec(range->spec.length) && range->elength < (range->spec.offset + range->spec.length)) {
        debugs(68, 2, "invalid (range is outside entity-length) content-range-spec near: '" << str << "'");
        return 0;
    }

    debugs(68, 8, "parsed content-range field: " <<
           (long int) range->spec.offset << "-" <<
           (long int) range->spec.offset + range->spec.length - 1 << " / " <<
           (long int) range->elength);

    return 1;
}