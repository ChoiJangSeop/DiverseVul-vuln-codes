Pl_ASCIIHexDecoder::write(unsigned char* buf, size_t len)
{
    if (this->eod)
    {
	return;
    }
    for (size_t i = 0; i < len; ++i)
    {
	char ch = toupper(buf[i]);
	switch (ch)
	{
	  case ' ':
	  case '\f':
	  case '\v':
	  case '\t':
	  case '\r':
	  case '\n':
	    QTC::TC("libtests", "Pl_ASCIIHexDecoder ignore space");
	    // ignore whitespace
	    break;

	  case '>':
	    this->eod = true;
	    flush();
	    break;

	  default:
	    if (((ch >= '0') && (ch <= '9')) ||
		((ch >= 'A') && (ch <= 'F')))
	    {
		this->inbuf[this->pos++] = ch;
		if (this->pos == 2)
		{
		    flush();
		}
	    }
	    else
	    {
		char t[2];
		t[0] = ch;
		t[1] = 0;
		throw std::runtime_error(
		    std::string("character out of range"
				" during base Hex decode: ") + t);
	    }
	    break;
	}
	if (this->eod)
	{
	    break;
	}
    }
}