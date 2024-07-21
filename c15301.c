BitStream::getBitsSigned(int nbits)
{
    unsigned long long bits = read_bits(this->p, this->bit_offset,
                                        this->bits_available, nbits);
    long long result = 0;
    if (static_cast<long long>(bits) > 1 << (nbits - 1))
    {
        result = static_cast<long long>(bits - (1 << nbits));
    }
    else
    {
        result = static_cast<long long>(bits);
    }
    return result;
}