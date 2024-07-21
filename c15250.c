QUtil::toUTF8(unsigned long uval)
{
    std::string result;

    // A UTF-8 encoding of a Unicode value is a single byte for
    // Unicode values <= 127.  For larger values, the first byte of
    // the UTF-8 encoding has '1' as each of its n highest bits and
    // '0' for its (n+1)th highest bit where n is the total number of
    // bytes required.  Subsequent bytes start with '10' and have the
    // remaining 6 bits free for encoding.  For example, an 11-bit
    // Unicode value can be stored in two bytes where the first is
    // 110zzzzz, the second is 10zzzzzz, and the z's represent the
    // remaining bits.

    if (uval > 0x7fffffff)
    {
	throw std::runtime_error("bounds error in QUtil::toUTF8");
    }
    else if (uval < 128)
    {
	result += static_cast<char>(uval);
    }
    else
    {
	unsigned char bytes[7];
	bytes[6] = '\0';
	unsigned char* cur_byte = &bytes[5];

	// maximum value that will fit in the current number of bytes
	unsigned char maxval = 0x3f; // six bits

	while (uval > maxval)
	{
	    // Assign low six bits plus 10000000 to lowest unused
	    // byte position, then shift
	    *cur_byte = static_cast<unsigned char>(0x80 + (uval & 0x3f));
	    uval >>= 6;
	    // Maximum that will fit in high byte now shrinks by one bit
	    maxval >>= 1;
	    // Slide to the left one byte
	    if (cur_byte <= bytes)
	    {
		throw std::logic_error("QUtil::toUTF8: overflow error");
	    }
	    --cur_byte;
	}
	// If maxval is k bits long, the high (7 - k) bits of the
	// resulting byte must be high.
	*cur_byte = static_cast<unsigned char>(
            (0xff - (1 + (maxval << 1))) + uval);

	result += reinterpret_cast<char*>(cur_byte);
    }

    return result;
}