Pl_LZWDecoder::sendNextCode()
{
    int high = this->byte_pos;
    int med = (this->byte_pos + 1) % 3;
    int low = (this->byte_pos + 2) % 3;

    int bits_from_high = 8 - this->bit_pos;
    int bits_from_med = this->code_size - bits_from_high;
    int bits_from_low = 0;
    if (bits_from_med > 8)
    {
	bits_from_low = bits_from_med - 8;
	bits_from_med = 8;
    }
    int high_mask = (1 << bits_from_high) - 1;
    int med_mask = 0xff - ((1 << (8 - bits_from_med)) - 1);
    int low_mask = 0xff - ((1 << (8 - bits_from_low)) - 1);
    int code = 0;
    code += (this->buf[high] & high_mask) << bits_from_med;
    code += ((this->buf[med] & med_mask) >> (8 - bits_from_med));
    if (bits_from_low)
    {
	code <<= bits_from_low;
	code += ((this->buf[low] & low_mask) >> (8 - bits_from_low));
	this->byte_pos = low;
	this->bit_pos = bits_from_low;
    }
    else
    {
	this->byte_pos = med;
	this->bit_pos = bits_from_med;
    }
    if (this->bit_pos == 8)
    {
	this->bit_pos = 0;
	++this->byte_pos;
	this->byte_pos %= 3;
    }
    this->bits_available -= this->code_size;

    handleCode(code);
}