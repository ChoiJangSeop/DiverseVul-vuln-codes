memxor_different_alignment (word_t *dst, const char *src, size_t n)
{
  int shl, shr;
  const word_t *src_word;
  unsigned offset = ALIGN_OFFSET (src);
  word_t s0, s1;

  shl = CHAR_BIT * offset;
  shr = CHAR_BIT * (sizeof(word_t) - offset);

  src_word = (const word_t *) ((uintptr_t) src & -sizeof(word_t));

  if (n & 1)
    {
      n--;
      s1 = src_word[n];
      s0 = src_word[n+1]; /* FIXME: Overread */
      dst[n] ^= MERGE (s1, shl, s0, shr);
    }
  else
    s1 = src_word[n]; /* FIXME: Overread */

  while (n > 0)
    {
      n -= 2;
      s0 = src_word[n+1];
      dst[n+1] ^= MERGE(s0, shl, s1, shr);
      s1 = src_word[n]; /* FIXME: Overread on last iteration */
      dst[n] ^= MERGE(s1, shl, s0, shr);
    }
}