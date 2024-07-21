BitStream::getBits(int nbits)
{
    return read_bits(this->p, this->bit_offset,
		     this->bits_available, nbits);
}