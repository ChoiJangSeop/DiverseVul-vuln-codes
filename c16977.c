u32tochar (wc, s)
     wchar_t wc;
     char *s;
{
  unsigned long x;
  int l;

  x = wc;
  l = (x <= UCHAR_MAX) ? 1 : ((x <= USHORT_MAX) ? 2 : 4);

  if (x <= UCHAR_MAX)
    s[0] = x & 0xFF;
  else if (x <= USHORT_MAX)	/* assume unsigned short = 16 bits */
    {
      s[0] = (x >> 8) & 0xFF;
      s[1] = x & 0xFF;
    }
  else
    {
      s[0] = (x >> 24) & 0xFF;
      s[1] = (x >> 16) & 0xFF;
      s[2] = (x >> 8) & 0xFF;
      s[3] = x & 0xFF;
    }
  s[l] = '\0';
  return l;  
}