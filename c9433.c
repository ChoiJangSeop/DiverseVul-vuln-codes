HttpHeader::operator =(const HttpHeader &other)
{
    if (this != &other) {
        // we do not really care, but the caller probably does
        assert(owner == other.owner);
        clean();
        update(&other); // will update the mask as well
        len = other.len;
        conflictingContentLength_ = other.conflictingContentLength_;
    }
    return *this;
}