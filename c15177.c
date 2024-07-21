Pl_AES_PDF::finish()
{
    if (this->encrypt)
    {
	if (this->offset == this->buf_size)
	{
	    flush(false);
	}
        if (! this->disable_padding)
        {
            // Pad as described in section 3.5.1 of version 1.7 of the PDF
            // specification, including providing an entire block of padding
            // if the input was a multiple of 16 bytes.
            unsigned char pad =
                static_cast<unsigned char>(this->buf_size - this->offset);
            memset(this->inbuf + this->offset, pad, pad);
            this->offset = this->buf_size;
            flush(false);
        }
    }
    else
    {
	if (this->offset != this->buf_size)
	{
	    // This is never supposed to happen as the output is
	    // always supposed to be padded.  However, we have
	    // encountered files for which the output is not a
	    // multiple of the block size.  In this case, pad with
	    // zeroes and hope for the best.
	    assert(this->buf_size > this->offset);
	    std::memset(this->inbuf + this->offset, 0,
			this->buf_size - this->offset);
	    this->offset = this->buf_size;
	}
	flush(! this->disable_padding);
    }
    getNext()->finish();
}