memxor(void *dst_in, const void *src_in, size_t n)
{
  char *dst = dst_in;
  const char *src = src_in;

  if (n >= WORD_T_THRESH)
    {
      unsigned i;
      unsigned offset;
      size_t nwords;
      /* There are at least some bytes to compare.  No need to test
	 for N == 0 in this alignment loop.  */
      for (i = ALIGN_OFFSET(dst + n); i > 0; i--)
	{
	  n--;
	  dst[n] ^= src[n];
	}
      offset = ALIGN_OFFSET(src + n);
      nwords = n / sizeof (word_t);
      n %= sizeof (word_t);

      if (offset)
	memxor_different_alignment ((word_t *) (dst+n), (src+n), nwords);
      else
	memxor_common_alignment ((word_t *) (dst+n),
				 (const word_t *) (src+n), nwords);
    }
  while (n > 0)
    {
      n--;
      dst[n] ^= src[n];
    }

  return dst;
}