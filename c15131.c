int rijndaelSetupEncrypt(u32 *rk, const u8 *key, int keybits)
{
  int i = 0;
  u32 temp;

  rk[0] = GETU32(key     );
  rk[1] = GETU32(key +  4);
  rk[2] = GETU32(key +  8);
  rk[3] = GETU32(key + 12);
  if (keybits == 128)
  {
    for (;;)
    {
      temp  = rk[3];
      rk[4] = rk[0] ^
        (Te4[(temp >> 16) & 0xff] & 0xff000000) ^
        (Te4[(temp >>  8) & 0xff] & 0x00ff0000) ^
        (Te4[(temp      ) & 0xff] & 0x0000ff00) ^
        (Te4[(temp >> 24)       ] & 0x000000ff) ^
        rcon[i];
      rk[5] = rk[1] ^ rk[4];
      rk[6] = rk[2] ^ rk[5];
      rk[7] = rk[3] ^ rk[6];
      if (++i == 10)
        return 10;
      rk += 4;
    }
  }
  rk[4] = GETU32(key + 16);
  rk[5] = GETU32(key + 20);
  if (keybits == 192)
  {
    for (;;)
    {
      temp = rk[ 5];
      rk[ 6] = rk[ 0] ^
        (Te4[(temp >> 16) & 0xff] & 0xff000000) ^
        (Te4[(temp >>  8) & 0xff] & 0x00ff0000) ^
        (Te4[(temp      ) & 0xff] & 0x0000ff00) ^
        (Te4[(temp >> 24)       ] & 0x000000ff) ^
        rcon[i];
      rk[ 7] = rk[ 1] ^ rk[ 6];
      rk[ 8] = rk[ 2] ^ rk[ 7];
      rk[ 9] = rk[ 3] ^ rk[ 8];
      if (++i == 8)
        return 12;
      rk[10] = rk[ 4] ^ rk[ 9];
      rk[11] = rk[ 5] ^ rk[10];
      rk += 6;
    }
  }
  rk[6] = GETU32(key + 24);
  rk[7] = GETU32(key + 28);
  if (keybits == 256)
  {
    for (;;)
    {
      temp = rk[ 7];
      rk[ 8] = rk[ 0] ^
        (Te4[(temp >> 16) & 0xff] & 0xff000000) ^
        (Te4[(temp >>  8) & 0xff] & 0x00ff0000) ^
        (Te4[(temp      ) & 0xff] & 0x0000ff00) ^
        (Te4[(temp >> 24)       ] & 0x000000ff) ^
        rcon[i];
      rk[ 9] = rk[ 1] ^ rk[ 8];
      rk[10] = rk[ 2] ^ rk[ 9];
      rk[11] = rk[ 3] ^ rk[10];
      if (++i == 7)
        return 14;
      temp = rk[11];
      rk[12] = rk[ 4] ^
        (Te4[(temp >> 24)       ] & 0xff000000) ^
        (Te4[(temp >> 16) & 0xff] & 0x00ff0000) ^
        (Te4[(temp >>  8) & 0xff] & 0x0000ff00) ^
        (Te4[(temp      ) & 0xff] & 0x000000ff);
      rk[13] = rk[ 5] ^ rk[12];
      rk[14] = rk[ 6] ^ rk[13];
      rk[15] = rk[ 7] ^ rk[14];
      rk += 8;
    }
  }
  return 0;
}