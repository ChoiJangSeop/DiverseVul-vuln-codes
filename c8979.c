spa_bits_to_base64 (uschar *out, const uschar *in, int inlen)
/* raw bytes in quasi-big-endian order to base 64 string (NUL-terminated) */
{
  for (; inlen >= 3; inlen -= 3)
    {
      *out++ = base64digits[in[0] >> 2];
      *out++ = base64digits[((in[0] << 4) & 0x30) | (in[1] >> 4)];
      *out++ = base64digits[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
      *out++ = base64digits[in[2] & 0x3f];
      in += 3;
    }
  if (inlen > 0)
    {
      uschar fragment;

      *out++ = base64digits[in[0] >> 2];
      fragment = (in[0] << 4) & 0x30;
      if (inlen > 1)
       fragment |= in[1] >> 4;
      *out++ = base64digits[fragment];
      *out++ = (inlen < 2) ? '=' : base64digits[(in[1] << 2) & 0x3c];
      *out++ = '=';
    }
  *out = '\0';
}