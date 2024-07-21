void MD5::update(unsigned char *input,
		 unsigned int inputLen)
{
    unsigned int i, index, partLen;

    // Compute number of bytes mod 64
    index = static_cast<unsigned int>((count[0] >> 3) & 0x3f);

    // Update number of bits
    if ((count[0] += (static_cast<UINT4>(inputLen) << 3)) <
        (static_cast<UINT4>(inputLen) << 3))
	count[1]++;
    count[1] += (static_cast<UINT4>(inputLen) >> 29);

    partLen = 64 - index;

    // Transform as many times as possible.

    if (inputLen >= partLen) {
	memcpy(&buffer[index], input, partLen);
	transform(state, buffer);

	for (i = partLen; i + 63 < inputLen; i += 64)
	    transform(state, &input[i]);

	index = 0;
    }
    else
	i = 0;

    // Buffer remaining input
    memcpy(&buffer[index], &input[i], inputLen-i);
}