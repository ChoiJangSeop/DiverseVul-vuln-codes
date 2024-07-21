BitWriter::writeBits(unsigned long long val, unsigned int bits)
{
    write_bits(this->ch, this->bit_offset, val, bits, this->pl);
}