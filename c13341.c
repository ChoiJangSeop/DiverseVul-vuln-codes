bit_write_UMC (Bit_Chain *dat, BITCODE_UMC val)
{
  int i, j;
  int negative;
  unsigned char byte[5];
  BITCODE_UMC mask;
  BITCODE_UMC value;

  value = val;
  mask = 0x0000007f;
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
  for (j = 4; j >= i; j--)
    bit_write_RC (dat, byte[j]);
}