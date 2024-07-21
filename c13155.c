ecma_snapshot_get_literal (const uint8_t *literal_base_p, /**< literal start */
                           ecma_value_t literal_value) /**< string / number offset */
{
  JERRY_ASSERT ((literal_value & ECMA_VALUE_TYPE_MASK) == ECMA_TYPE_SNAPSHOT_OFFSET);

  const uint8_t *literal_p = literal_base_p + (literal_value >> JERRY_SNAPSHOT_LITERAL_SHIFT);

  if (literal_value & JERRY_SNAPSHOT_LITERAL_IS_NUMBER)
  {
    ecma_number_t num;
    memcpy (&num, literal_p, sizeof (ecma_number_t));
    return ecma_find_or_create_literal_number (num);
  }

#if JERRY_BUILTIN_BIGINT
  if (literal_value & JERRY_SNAPSHOT_LITERAL_IS_BIGINT)
  {
    uint32_t bigint_sign_and_size = *(uint32_t *) literal_p;
    uint32_t size = bigint_sign_and_size & ~(uint32_t) (sizeof (ecma_bigint_digit_t) - 1);

    ecma_extended_primitive_t *bigint_p = ecma_bigint_create (size);

    if (bigint_p == NULL)
    {
      jerry_fatal (ERR_OUT_OF_MEMORY);
    }

    /* Only the sign bit can differ. */
    JERRY_ASSERT (bigint_p->u.bigint_sign_and_size == (bigint_sign_and_size & ~(uint32_t) ECMA_BIGINT_SIGN));

    bigint_p->u.bigint_sign_and_size = bigint_sign_and_size;
    memcpy (ECMA_BIGINT_GET_DIGITS (bigint_p, 0), literal_p + sizeof (uint32_t), size);
    return ecma_find_or_create_literal_bigint (ecma_make_extended_primitive_value (bigint_p, ECMA_TYPE_BIGINT));
  }
#endif /* JERRY_BUILTIN_BIGINT */

  uint16_t length = *(const uint16_t *) literal_p;

  return ecma_find_or_create_literal_string (literal_p + sizeof (uint16_t), length);
} /* ecma_snapshot_get_literal */