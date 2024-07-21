bit_write_MC (Bit_Chain *dat, BITCODE_MC val)
{
  int i, j;
  int negative = 0;
  unsigned char byte[5];
  BITCODE_UMC mask = 0x0000007f;
  BITCODE_UMC value = (BITCODE_UMC)val;

  if (val < 0)
    {
      negative = 1;
      value = (BITCODE_UMC)-val;
    }
  for (i = 4, j = 0; i >= 0; i--, j += 7)
    {
      byte[i] = (unsigned char)((value & mask) >> j);
      byte[i] |= 0x80;
      mask = mask << 7;
    }
  for (i = 0; i < 4; i++)
    if (byte[i] & 0x7f)
      break;

  if (byte[i] & 0x40)
    i--;
  byte[i] &= 0x7f;
  if (negative)
    byte[i] |= 0x40;
  for (j = 4; j >= i; j--)
    bit_write_RC (dat, byte[j]);
}