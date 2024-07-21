int mutt_from_base64 (char *out, const char *in)
{
  int len = 0;
  register unsigned char digit1, digit2, digit3, digit4;

  do
  {
    digit1 = in[0];
    if (digit1 > 127 || base64val (digit1) == BAD)
      return -1;
    digit2 = in[1];
    if (digit2 > 127 || base64val (digit2) == BAD)
      return -1;
    digit3 = in[2];
    if (digit3 > 127 || ((digit3 != '=') && (base64val (digit3) == BAD)))
      return -1;
    digit4 = in[3];
    if (digit4 > 127 || ((digit4 != '=') && (base64val (digit4) == BAD)))
      return -1;
    in += 4;

    /* digits are already sanity-checked */
    *out++ = (base64val(digit1) << 2) | (base64val(digit2) >> 4);
    len++;
    if (digit3 != '=')
    {
      *out++ = ((base64val(digit2) << 4) & 0xf0) | (base64val(digit3) >> 2);
      len++;
      if (digit4 != '=')
      {
	*out++ = ((base64val(digit3) << 6) & 0xc0) | base64val(digit4);
	len++;
      }
    }
  }
  while (*in && digit4 != '=');

  return len;
}