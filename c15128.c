BitWriter::writeBitsSigned(long long val, unsigned int bits)
{
    unsigned long long uval = 0;
    if (val < 0)
    {
        uval = static_cast<unsigned long long>((1 << bits) + val);
    }
    else
    {
        uval = static_cast<unsigned long long>(val);
    }
    writeBits(uval, bits);
}