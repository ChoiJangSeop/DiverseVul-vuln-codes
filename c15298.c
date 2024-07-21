void MD5::decode(UINT4 *output, unsigned char *input, unsigned int len)
{
    unsigned int i, j;

    for (i = 0, j = 0; j < len; i++, j += 4)
	output[i] =
            static_cast<UINT4>(input[j]) |
            (static_cast<UINT4>(input[j+1]) << 8) |
	    (static_cast<UINT4>(input[j+2]) << 16) |
            (static_cast<UINT4>(input[j+3]) << 24);
}