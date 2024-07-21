ecma_find_or_create_literal_string (const lit_utf8_byte_t *chars_p, /**< string to be searched */
                                    lit_utf8_size_t size) /**< size of the string */
{
  ecma_string_t *string_p = ecma_new_ecma_string_from_utf8 (chars_p, size);

  if (ECMA_IS_DIRECT_STRING (string_p))
  {
    return ecma_make_string_value (string_p);
  }

  jmem_cpointer_t string_list_cp = JERRY_CONTEXT (string_list_first_cp);
  jmem_cpointer_t *empty_cpointer_p = NULL;

  while (string_list_cp != JMEM_CP_NULL)
  {
    ecma_lit_storage_item_t *string_list_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_lit_storage_item_t, string_list_cp);

    for (int i = 0; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
    {
      if (string_list_p->values[i] == JMEM_CP_NULL)
      {
        if (empty_cpointer_p == NULL)
        {
          empty_cpointer_p = string_list_p->values + i;
        }
      }
      else
      {
        ecma_string_t *value_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t,
                                                               string_list_p->values[i]);

        if (ecma_compare_ecma_strings (string_p, value_p))
        {
          /* Return with string if found in the list. */
          ecma_deref_ecma_string (string_p);
          return ecma_make_string_value (value_p);
        }
      }
    }

    string_list_cp = string_list_p->next_cp;
  }

  ECMA_SET_STRING_AS_STATIC (string_p);
  jmem_cpointer_t result;
  JMEM_CP_SET_NON_NULL_POINTER (result, string_p);

  if (empty_cpointer_p != NULL)
  {
    *empty_cpointer_p = result;
    return ecma_make_string_value (string_p);
  }

  ecma_lit_storage_item_t *new_item_p;
  new_item_p = (ecma_lit_storage_item_t *) jmem_pools_alloc (sizeof (ecma_lit_storage_item_t));

  new_item_p->values[0] = result;
  for (int i = 1; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
  {
    new_item_p->values[i] = JMEM_CP_NULL;
  }

  new_item_p->next_cp = JERRY_CONTEXT (string_list_first_cp);
  JMEM_CP_SET_NON_NULL_POINTER (JERRY_CONTEXT (string_list_first_cp), new_item_p);

  return ecma_make_string_value (string_p);
} /* ecma_find_or_create_literal_string */