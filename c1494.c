LibRaw_byte_buffer *LibRaw_abstract_datastream::make_byte_buffer(unsigned int sz)
{
    LibRaw_byte_buffer *ret = new LibRaw_byte_buffer(sz);
    read(ret->get_buffer(),sz,1);
    return ret;
}