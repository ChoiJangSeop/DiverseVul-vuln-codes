Pl_LZWDecoder::addToTable(unsigned char next)
{
    unsigned int last_size = 0;
    unsigned char const* last_data = 0;
    unsigned char tmp[1];

    if (this->last_code < 256)
    {
	tmp[0] = this->last_code;
	last_data = tmp;
	last_size = 1;
    }
    else if (this->last_code > 257)
    {
	unsigned int idx = this->last_code - 258;
	if (idx >= table.size())
        {
            throw std::logic_error(
                "Pl_LZWDecoder::addToTable: table overflow");
        }
	Buffer& b = table.at(idx);
	last_data = b.getBuffer();
	last_size = b.getSize();
    }
    else
    {
        throw std::logic_error(
            "Pl_LZWDecoder::addToTable called with invalid code (" +
            QUtil::int_to_string(this->last_code) + ")");
    }

    Buffer entry(1 + last_size);
    unsigned char* new_data = entry.getBuffer();
    memcpy(new_data, last_data, last_size);
    new_data[last_size] = next;
    this->table.push_back(entry);
}