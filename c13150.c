ecma_bigint_to_string (ecma_value_t value, /**< BigInt value */
                       ecma_bigint_digit_t radix) /**< conversion radix */
{
  JERRY_ASSERT (ecma_is_value_bigint (value));

  if (value == ECMA_BIGINT_ZERO)
  {
    return ecma_new_ecma_string_from_code_unit (LIT_CHAR_0);
  }

  uint32_t char_start_p, char_size_p;
  ecma_extended_primitive_t *bigint_p = ecma_get_extended_primitive_from_value (value);
  lit_utf8_byte_t *string_buffer_p = ecma_big_uint_to_string (bigint_p, radix, &char_start_p, &char_size_p);

  if (JERRY_UNLIKELY (string_buffer_p == NULL))
  {
    ecma_raise_range_error (ECMA_ERR_MSG ("Cannot allocate memory for a string representation of a BigInt value"));
    return NULL;
  }

  JERRY_ASSERT (char_start_p > 0);

  if (bigint_p->u.bigint_sign_and_size & ECMA_BIGINT_SIGN)
  {
    string_buffer_p[--char_start_p] = LIT_CHAR_MINUS;
  }

  ecma_string_t *string_p;
  string_p = ecma_new_ecma_string_from_utf8 (string_buffer_p + char_start_p, char_size_p - char_start_p);

  jmem_heap_free_block (string_buffer_p, char_size_p);
  return string_p;
} /* ecma_bigint_to_string */