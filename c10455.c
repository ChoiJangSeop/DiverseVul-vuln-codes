HeaderLookupTable_t::lookup (const char *buf, const std::size_t len) const {
    const HeaderTableRecord *r = HttpHeaderHashTable::lookup(buf, len);
    if (!r)
        return BadHdr;
    return *r;
}