memxor3_different_alignment_b (word_t *dst,
			       const word_t *a, const char *b, unsigned offset, size_t n)
{
  int shl, shr;
  const word_t *b_word;

  word_t s0, s1;

  shl = CHAR_BIT * offset;
  shr = CHAR_BIT * (sizeof(word_t) - offset);

  b_word = (const word_t *) ((uintptr_t) b & -sizeof(word_t));

  if (n & 1)
    {
      n--;
      s1 = b_word[n];
      s0 = b_word[n+1];
      dst[n] = a[n] ^ MERGE (s1, shl, s0, shr);
    }
  else
    s1 = b_word[n];

  while (n > 0)
    {
      n -= 2;
      s0 = b_word[n+1];
      dst[n+1] = a[n+1] ^ MERGE(s0, shl, s1, shr);
      s1 = b_word[n];
      dst[n] = a[n] ^ MERGE(s1, shl, s0, shr);
    }
}