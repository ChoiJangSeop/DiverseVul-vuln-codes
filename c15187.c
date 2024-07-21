test_read_bits(unsigned char const* buf,
	       unsigned char const*& p, unsigned int& bit_offset,
	       unsigned int& bits_available, int bits_wanted)
{
    unsigned long result =
	read_bits(p, bit_offset, bits_available, bits_wanted);

    std::cout << "bits read: " << bits_wanted << ", result = " << result
	      << std::endl;
    print_values(p - buf, bit_offset, bits_available);
}