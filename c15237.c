BufferInputSource::findAndSkipNextEOL()
{
    if (this->cur_offset < 0)
    {
        throw std::logic_error("INTERNAL ERROR: BufferInputSource offset < 0");
    }
    qpdf_offset_t end_pos = this->buf->getSize();
    if (this->cur_offset >= end_pos)
    {
	this->last_offset = end_pos;
        this->cur_offset = end_pos;
	return end_pos;
    }

    qpdf_offset_t result = 0;
    size_t len = end_pos - this->cur_offset;
    unsigned char const* buffer = this->buf->getBuffer();

    void* start = const_cast<unsigned char*>(buffer) + this->cur_offset;
    unsigned char* p1 = static_cast<unsigned char*>(memchr(start, '\r', len));
    unsigned char* p2 = static_cast<unsigned char*>(memchr(start, '\n', len));
    unsigned char* p = (p1 && p2) ? std::min(p1, p2) : p1 ? p1 : p2;
    if (p)
    {
        result = p - buffer;
        this->cur_offset = result + 1;
        ++p;
        while ((this->cur_offset < end_pos) &&
               ((*p == '\r') || (*p == '\n')))
        {
            ++p;
            ++this->cur_offset;
        }
    }
    else
    {
        this->cur_offset = end_pos;
        result = end_pos;
    }
    return result;
}