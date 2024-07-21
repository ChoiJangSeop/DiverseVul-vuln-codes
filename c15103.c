RC4::RC4(unsigned char const* key_data, int key_len)
{
    if (key_len == -1)
    {
	key_len = strlen(reinterpret_cast<char const*>(key_data));
    }

    for (int i = 0; i < 256; ++i)
    {
        key.state[i] = i;
    }
    key.x = 0;
    key.y = 0;

    int i1 = 0;
    int i2 = 0;
    for (int i = 0; i < 256; ++i)
    {
	i2 = (key_data[i1] + key.state[i] + i2) % 256;
	swap_byte(key.state[i], key.state[i2]);
	i1 = (i1 + 1) % key_len;
    }
}