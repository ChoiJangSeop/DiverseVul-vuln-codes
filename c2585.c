memxor3(void *dst_in, const void *a_in, const void *b_in, size_t n)
{
  char *dst = dst_in;
  const char *a = a_in;
  const char *b = b_in;

  if (n >= WORD_T_THRESH)
    {
      unsigned i;
      unsigned a_offset;
      unsigned b_offset;
      size_t nwords;

      for (i = ALIGN_OFFSET(dst + n); i > 0; i--)
	{
	  n--;
	  dst[n] = a[n] ^ b[n];
	}

      a_offset = ALIGN_OFFSET(a + n);
      b_offset = ALIGN_OFFSET(b + n);

      nwords = n / sizeof (word_t);
      n %= sizeof (word_t);

      if (a_offset == b_offset)
	{
	  if (!a_offset)
	    memxor3_common_alignment((word_t *) (dst + n),
				     (const word_t *) (a + n),
				     (const word_t *) (b + n), nwords);
	  else
	    memxor3_different_alignment_ab((word_t *) (dst + n),
					   a + n, b + n, a_offset,
					   nwords);
	}
      else if (!a_offset)
	memxor3_different_alignment_b((word_t *) (dst + n),
				      (const word_t *) (a + n), b + n,
				      b_offset, nwords);
      else if (!b_offset)
	memxor3_different_alignment_b((word_t *) (dst + n),
				      (const word_t *) (b + n), a + n,
				      a_offset, nwords);
      else
	memxor3_different_alignment_all((word_t *) (dst + n), a + n, b + n,
					a_offset, b_offset, nwords);

    }
  while (n-- > 0)
    dst[n] = a[n] ^ b[n];

  return dst;
}