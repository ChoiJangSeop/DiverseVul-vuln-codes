BitStream::reset()
{
    p = start;
    bit_offset = 7;
    if (static_cast<unsigned int>(nbytes) > static_cast<unsigned int>(-1) / 8)
    {
	throw std::runtime_error("array too large for bitstream");
    }
    bits_available = 8 * nbytes;
}