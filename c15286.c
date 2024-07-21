void MD5::encode(unsigned char *output, UINT4 *input, unsigned int len)
{
    unsigned int i, j;

    for (i = 0, j = 0; j < len; i++, j += 4) {
	output[j] = static_cast<unsigned char>(input[i] & 0xff);
	output[j+1] = static_cast<unsigned char>((input[i] >> 8) & 0xff);
	output[j+2] = static_cast<unsigned char>((input[i] >> 16) & 0xff);
	output[j+3] = static_cast<unsigned char>((input[i] >> 24) & 0xff);
    }
}