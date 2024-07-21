scanner_append_argument (parser_context_t *context_p, /**< context */
                         scanner_context_t *scanner_context_p) /**< scanner context */
{
  scanner_literal_pool_t *literal_pool_p = scanner_context_p->active_literal_pool_p;
  parser_list_iterator_t literal_iterator;
  parser_list_iterator_init (&literal_pool_p->literal_pool, &literal_iterator);
  lexer_lit_location_t *literal_location_p = &context_p->token.lit_location;
  lexer_lit_location_t *literal_p;

  const uint8_t *char_p = literal_location_p->char_p;
  prop_length_t length = literal_location_p->length;

  JERRY_ASSERT (SCANNER_LITERAL_POOL_MAY_HAVE_ARGUMENTS (literal_pool_p->status_flags));

  if (JERRY_LIKELY (!context_p->token.lit_location.has_escape))
  {
    while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
    {
      if (literal_p->length == length)
      {
        if (JERRY_LIKELY (!literal_p->has_escape))
        {
          if (memcmp (literal_p->char_p, char_p, length) == 0)
          {
            break;
          }
        }
        else if (lexer_compare_identifier_to_string (literal_p, char_p, length))
        {
          break;
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
        break;
      }
    }
  }

  uint8_t literal_type = SCANNER_LITERAL_IS_ARG;

  if (literal_p != NULL)
  {
    literal_p->length = 0;

#if JERRY_ESNEXT
    if (literal_p->type & SCANNER_LITERAL_IS_USED)
    {
      literal_type = SCANNER_LITERAL_IS_ARG | SCANNER_LITERAL_EARLY_CREATE;
    }
#endif /* JERRY_ESNEXT */
  }

  literal_p = (lexer_lit_location_t *) parser_list_append (context_p, &literal_pool_p->literal_pool);

  *literal_p = context_p->token.lit_location;
  literal_p->type = literal_type;

  return literal_p;
} /* scanner_append_argument */