Pl_Count::write(unsigned char* buf, size_t len)
{
    if (len)
    {
	this->count += len;
	getNext()->write(buf, len);
	this->last_char = buf[len - 1];
    }
}