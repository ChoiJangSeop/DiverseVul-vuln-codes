iterate_rc4(unsigned char* data, int data_len,
	    unsigned char* okey, int key_len,
	    int iterations, bool reverse)
{
    unsigned char* key = new unsigned char[key_len];
    for (int i = 0; i < iterations; ++i)
    {
	int const xor_value = (reverse ? iterations - 1 - i : i);
	for (int j = 0; j < key_len; ++j)
	{
	    key[j] = okey[j] ^ xor_value;
	}
	RC4 rc4(key, key_len);
	rc4.process(data, data_len);
    }
    delete [] key;
}