Pl_LZWDecoder::getFirstChar(int code)
{
    unsigned char result = '\0';
    if (code < 256)
    {
	result = static_cast<unsigned char>(code);
    }
    else if (code > 257)
    {
	unsigned int idx = code - 258;
	if (idx >= table.size())
        {
            throw std::logic_error(
                "Pl_LZWDecoder::getFirstChar: table overflow");
        }
	Buffer& b = table.at(idx);
	result = b.getBuffer()[0];
    }
    else
    {
        throw std::logic_error(
            "Pl_LZWDecoder::getFirstChar called with invalid code (" +
            QUtil::int_to_string(code) + ")");
    }
    return result;
}