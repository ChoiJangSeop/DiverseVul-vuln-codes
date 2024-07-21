test_write_bits(unsigned char& ch, unsigned int& bit_offset, unsigned long val,
		int bits, Pl_Buffer* bp)
{
    write_bits(ch, bit_offset, val, bits, bp);
    printf("ch = %02x, bit_offset = %d\n",
           static_cast<unsigned int>(ch), bit_offset);
}