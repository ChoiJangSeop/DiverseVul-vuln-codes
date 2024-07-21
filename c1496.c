LibRaw_byte_buffer *LibRaw_buffer_datastream::make_byte_buffer(unsigned int sz)
{
    LibRaw_byte_buffer *ret = new LibRaw_byte_buffer(0);
    if(streampos + sz > streamsize)
        sz = streamsize - streampos;
    ret->set_buffer(buf+streampos,sz);
    return ret;
}