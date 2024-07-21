BufferInputSource::seek(qpdf_offset_t offset, int whence)
{
    switch (whence)
    {
      case SEEK_SET:
	this->cur_offset = offset;
	break;

      case SEEK_END:
	this->cur_offset = this->buf->getSize() + offset;
	break;

      case SEEK_CUR:
	this->cur_offset += offset;
	break;

      default:
	throw std::logic_error(
	    "INTERNAL ERROR: invalid argument to BufferInputSource::seek");
	break;
    }

    if (this->cur_offset < 0)
    {
        throw std::runtime_error(
            this->description + ": seek before beginning of buffer");
    }
}