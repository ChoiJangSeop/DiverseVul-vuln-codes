BufferInputSource::read(char* buffer, size_t length)
{
    if (this->cur_offset < 0)
    {
        throw std::logic_error("INTERNAL ERROR: BufferInputSource offset < 0");
    }
    qpdf_offset_t end_pos = this->buf->getSize();
    if (this->cur_offset >= end_pos)
    {
	this->last_offset = end_pos;
	return 0;
    }

    this->last_offset = this->cur_offset;
    size_t len = std::min(
        static_cast<size_t>(end_pos - this->cur_offset), length);
    memcpy(buffer, buf->getBuffer() + this->cur_offset, len);
    this->cur_offset += len;
    return len;
}