BitWriter::flush()
{
    if (bit_offset < 7)
    {
	int bits_to_write = bit_offset + 1;
	write_bits(this->ch, this->bit_offset, 0, bits_to_write, this->pl);
    }
}