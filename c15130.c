test()
{
    // 11110101 00010101 01100101 01111001 00010010 10001001 01110101 01001011
    // F5 15 65 79 12 89 75 4B

    // Read tests

    static unsigned char const buf[] = {
	0xF5, 0x15, 0x65, 0x79, 0x12, 0x89, 0x75, 0x4B
    };

    unsigned char const* p = buf;
    unsigned int bit_offset = 7;
    unsigned int bits_available = 64;

    // 11110:101 0:001010:1 01100101: 01111001
    // 0:00:1:0010 10001001 01110101 01001:011
    print_values(p - buf, bit_offset, bits_available);
    test_read_bits(buf, p, bit_offset, bits_available, 5);
    test_read_bits(buf, p, bit_offset, bits_available, 4);
    test_read_bits(buf, p, bit_offset, bits_available, 6);
    test_read_bits(buf, p, bit_offset, bits_available, 9);
    test_read_bits(buf, p, bit_offset, bits_available, 9);
    test_read_bits(buf, p, bit_offset, bits_available, 2);
    test_read_bits(buf, p, bit_offset, bits_available, 1);
    test_read_bits(buf, p, bit_offset, bits_available, 0);
    test_read_bits(buf, p, bit_offset, bits_available, 25);

    try
    {
	test_read_bits(buf, p, bit_offset, bits_available, 4);
    }
    catch (std::exception& e)
    {
	std::cout << "exception: " << e.what() << std::endl;
	print_values(p - buf, bit_offset, bits_available);
    }

    test_read_bits(buf, p, bit_offset, bits_available, 3);
    std::cout << std::endl;

    // 11110101 00010101 01100101 01111001: 00010010 10001001 01110101 01001011

    p = buf;
    bit_offset = 7;
    bits_available = 64;
    print_values(p - buf, bit_offset, bits_available);
    test_read_bits(buf, p, bit_offset, bits_available, 32);
    test_read_bits(buf, p, bit_offset, bits_available, 32);
    std::cout << std::endl;

    BitStream b(buf, 8);
    std::cout << b.getBits(32) << std::endl;
    b.reset();
    std::cout << b.getBits(32) << std::endl;
    std::cout << b.getBits(32) << std::endl;
    std::cout << std::endl;

    b.reset();
    std::cout << b.getBits(6) << std::endl;
    b.skipToNextByte();
    std::cout << b.getBits(8) << std::endl;
    b.skipToNextByte();
    std::cout << b.getBits(8) << std::endl;
    std::cout << std::endl;
    b.reset();
    std::cout << b.getBitsSigned(3) << std::endl;
    std::cout << b.getBitsSigned(6) << std::endl;
    std::cout << b.getBitsSigned(5) << std::endl;
    std::cout << b.getBitsSigned(1) << std::endl;
    std::cout << b.getBitsSigned(17) << std::endl;
    std::cout << std::endl;

    // Write tests

    // 11110:101 0:001010:1 01100101: 01111001
    // 0:00:1:0010 10001001 01110101 01001:011

    unsigned char ch = 0;
    bit_offset = 7;
    Pl_Buffer* bp = new Pl_Buffer("buffer");

    test_write_bits(ch, bit_offset, 30UL, 5, bp);
    test_write_bits(ch, bit_offset, 10UL, 4, bp);
    test_write_bits(ch, bit_offset, 10UL, 6, bp);
    test_write_bits(ch, bit_offset, 16059UL, 0, bp);
    test_write_bits(ch, bit_offset, 357UL, 9, bp);
    print_buffer(bp);

    test_write_bits(ch, bit_offset, 242UL, 9, bp);
    test_write_bits(ch, bit_offset, 0UL, 2, bp);
    test_write_bits(ch, bit_offset, 1UL, 1, bp);
    test_write_bits(ch, bit_offset, 5320361UL, 25, bp);
    test_write_bits(ch, bit_offset, 3UL, 3, bp);

    print_buffer(bp);
    test_write_bits(ch, bit_offset, 4111820153UL, 32, bp);
    test_write_bits(ch, bit_offset, 310998347UL, 32, bp);
    print_buffer(bp);

    BitWriter bw(bp);
    bw.writeBits(30UL, 5);
    bw.flush();
    bw.flush();
    bw.writeBits(0xABUL, 8);
    bw.flush();
    print_buffer(bp);
    bw.writeBitsSigned(-1, 3);  // 111
    bw.writeBitsSigned(-12, 6); // 110100
    bw.writeBitsSigned(4, 3);   // 100
    bw.writeBitsSigned(-4, 3);  // 100
    bw.writeBitsSigned(-1, 1);  // 1
    bw.flush();
    print_buffer(bp);

    delete bp;
}