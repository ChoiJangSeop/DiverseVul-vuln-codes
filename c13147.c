scanner_add_custom_literal (parser_context_t *context_p, /**< context */
                            scanner_literal_pool_t *literal_pool_p, /**< literal pool */
                            const lexer_lit_location_t *literal_location_p) /**< literal */
{
  while (true)
  {
    parser_list_iterator_t literal_iterator;
    parser_list_iterator_init (&literal_pool_p->literal_pool, &literal_iterator);
    lexer_lit_location_t *literal_p;

    const uint8_t *char_p = literal_location_p->char_p;
    prop_length_t length = literal_location_p->length;

    if (JERRY_LIKELY (!literal_location_p->has_escape))
    {
      while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
      {
        if (literal_p->length == length)
        {
          if (JERRY_LIKELY (!literal_p->has_escape))
          {
            if (memcmp (literal_p->char_p, char_p, length) == 0)
            {
              return literal_p;
            }
          }
          else if (lexer_compare_identifier_to_string (literal_p, char_p, length))
          {
            /* The non-escaped version is preferred. */
            literal_p->char_p = char_p;
            literal_p->has_escape = 0;
            return literal_p;
          }
        }
      }
    }
    else
    {
      while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
      {
        if (lexer_compare_identifiers (context_p, literal_p, literal_location_p))
        {
          return literal_p;
        }
      }
    }

#if JERRY_ESNEXT
    if (JERRY_UNLIKELY (literal_pool_p->status_flags & SCANNER_LITERAL_POOL_CLASS_NAME))
    {
      literal_pool_p = literal_pool_p->prev_p;
      continue;
    }
#endif /* JERRY_ESNEXT */

    literal_p = (lexer_lit_location_t *) parser_list_append (context_p, &literal_pool_p->literal_pool);
    *literal_p = *literal_location_p;

    literal_p->type = 0;

    return literal_p;
  }
} /* scanner_add_custom_literal */