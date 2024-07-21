ecma_string_substr (const ecma_string_t *string_p, /**< pointer to an ecma string */
                    lit_utf8_size_t start_pos, /**< start position, should be less or equal than string length */
                    lit_utf8_size_t end_pos) /**< end position, should be less or equal than string length */
{
  const lit_utf8_size_t string_length = ecma_string_get_length (string_p);
  JERRY_ASSERT (start_pos <= string_length);
  JERRY_ASSERT (end_pos <= string_length);

  if (start_pos >= end_pos)
  {
    return ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
  }

  ecma_string_t *ecma_string_p = NULL;
  end_pos -= start_pos;

  ECMA_STRING_TO_UTF8_STRING (string_p, start_p, buffer_size);

  if (string_length == buffer_size)
  {
    ecma_string_p = ecma_new_ecma_string_from_utf8 (start_p + start_pos,
                                                    (lit_utf8_size_t) end_pos);
  }
  else
  {
    while (start_pos--)
    {
      start_p += lit_get_unicode_char_size_by_utf8_first_byte (*start_p);
    }

    const lit_utf8_byte_t *end_p = start_p;
    while (end_pos--)
    {
      end_p += lit_get_unicode_char_size_by_utf8_first_byte (*end_p);
    }

    ecma_string_p = ecma_new_ecma_string_from_utf8 (start_p, (lit_utf8_size_t) (end_p - start_p));
  }

  ECMA_FINALIZE_UTF8_STRING (start_p, buffer_size);

  return ecma_string_p;
} /* ecma_string_substr */