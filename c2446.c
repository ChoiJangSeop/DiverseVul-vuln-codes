do_find_tlv (const unsigned char *buffer, size_t length,
             int tag, size_t *nbytes, int nestlevel)
{
  const unsigned char *s = buffer;
  size_t n = length;
  size_t len;
  int this_tag;
  int composite;

  for (;;)
    {
      buffer = s;
      if (n < 2)
        return NULL; /* Buffer definitely too short for tag and length. */
      if (!*s || *s == 0xff)
        { /* Skip optional filler between TLV objects. */
          s++;
          n--;
          continue;
        }
      composite = !!(*s & 0x20);
      if ((*s & 0x1f) == 0x1f)
        { /* more tag bytes to follow */
          s++;
          n--;
          if (n < 2)
            return NULL; /* buffer definitely too short for tag and length. */
          if ((*s & 0x1f) == 0x1f)
            return NULL; /* We support only up to 2 bytes. */
          this_tag = (s[-1] << 8) | (s[0] & 0x7f);
        }
      else
        this_tag = s[0];
      len = s[1];
      s += 2; n -= 2;
      if (len < 0x80)
        ;
      else if (len == 0x81)
        { /* One byte length follows. */
          if (!n)
            return NULL; /* we expected 1 more bytes with the length. */
          len = s[0];
          s++; n--;
        }
      else if (len == 0x82)
        { /* Two byte length follows. */
          if (n < 2)
            return NULL; /* We expected 2 more bytes with the length. */
          len = (s[0] << 8) | s[1];
          s += 2; n -= 2;
        }
      else
        return NULL; /* APDU limit is 65535, thus it does not make
                        sense to assume longer length fields. */

      if (composite && nestlevel < 100)
        { /* Dive into this composite DO after checking for a too deep
             nesting. */
          const unsigned char *tmp_s;
          size_t tmp_len;

          tmp_s = do_find_tlv (s, len, tag, &tmp_len, nestlevel+1);
          if (tmp_s)
            {
              *nbytes = tmp_len;
              return tmp_s;
            }
        }

      if (this_tag == tag)
        {
          *nbytes = len;
          return s;
        }
      if (len > n)
        return NULL; /* Buffer too short to skip to the next tag. */
      s += len; n -= len;
    }
}