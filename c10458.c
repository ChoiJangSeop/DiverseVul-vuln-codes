HttpHdrRange::contains(const HttpHdrRangeSpec& r) const
{
    assert(r.length >= 0);
    HttpHdrRangeSpec::HttpRange rrange(r.offset, r.offset + r.length);

    for (const_iterator i = begin(); i != end(); ++i) {
        HttpHdrRangeSpec::HttpRange irange((*i)->offset, (*i)->offset + (*i)->length);
        HttpHdrRangeSpec::HttpRange intersection = rrange.intersection(irange);

        if (intersection.start == irange.start && intersection.size() == irange.size())
            return true;
    }

    return false;
}