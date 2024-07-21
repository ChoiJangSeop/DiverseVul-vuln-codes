u32toutf8 (wc, s)
     u_bits32_t wc;
     char *s;
{
  int l;

  if (wc < 0x0080)
    {
      s[0] = (char)wc;
      l = 1;
    }
  else if (wc < 0x0800)
    {
      s[0] = (wc >> 6) | 0xc0;
      s[1] = (wc & 0x3f) | 0x80;
      l = 2;
    }
  else if (wc < 0x10000)
    {
      s[0] = (wc >> 12) | 0xe0;
      s[1] = ((wc >> 6) & 0x3f) | 0x80;
      s[2] = (wc & 0x3f) | 0x80;
      l = 3;
    }
  else if (wc < 0x200000)
    {
      s[0] = (wc >> 18) | 0xf0;
      s[1] = ((wc >> 12) & 0x3f) | 0x80;
      s[2] = ((wc >>  6) & 0x3f) | 0x80;
      s[3] = (wc & 0x3f) | 0x80;
      l = 4;
    }
  /* Strictly speaking, UTF-8 doesn't have characters longer than 4 bytes */
  else if (wc < 0x04000000)
    {
      s[0] = (wc >> 24) | 0xf8;
      s[1] = ((wc >> 18) & 0x3f) | 0x80;
      s[2] = ((wc >> 12) & 0x3f) | 0x80;
      s[3] = ((wc >>  6) & 0x3f) | 0x80;
      s[4] = (wc & 0x3f) | 0x80;
      l = 5;
    }
  else
    {
      s[0] = (wc >> 30) | 0xf8;
      s[1] = ((wc >> 24) & 0x3f) | 0x80;
      s[2] = ((wc >> 18) & 0x3f) | 0x80;
      s[3] = ((wc >> 12) & 0x3f) | 0x80;
      s[4] = ((wc >>  6) & 0x3f) | 0x80;
      s[5] = (wc & 0x3f) | 0x80;
      l = 6;
    }

  s[l] = '\0';
  return l;
}