scanner_detect_invalid_var (parser_context_t *context_p, /**< context */
                            scanner_context_t *scanner_context_p, /**< scanner context */
                            lexer_lit_location_t *var_literal_p) /**< var literal */
{
  if (var_literal_p->type & SCANNER_LITERAL_IS_LOCAL
      && !(var_literal_p->type & (SCANNER_LITERAL_IS_FUNC | SCANNER_LITERAL_IS_ARG))
      && (var_literal_p->type & SCANNER_LITERAL_IS_LOCAL) != SCANNER_LITERAL_IS_LOCAL)
  {
    scanner_raise_redeclaration_error (context_p);
  }

  scanner_literal_pool_t *literal_pool_p = scanner_context_p->active_literal_pool_p;

  if (!(literal_pool_p->status_flags & SCANNER_LITERAL_POOL_FUNCTION)
      && ((var_literal_p->type & SCANNER_LITERAL_IS_LOCAL_FUNC) == SCANNER_LITERAL_IS_LOCAL_FUNC))
  {
    scanner_raise_redeclaration_error (context_p);
  }

  const uint8_t *char_p = var_literal_p->char_p;
  prop_length_t length = var_literal_p->length;

  while (!(literal_pool_p->status_flags & SCANNER_LITERAL_POOL_FUNCTION))
  {
    literal_pool_p = literal_pool_p->prev_p;

    parser_list_iterator_t literal_iterator;
    parser_list_iterator_init (&literal_pool_p->literal_pool, &literal_iterator);
    lexer_lit_location_t *literal_p;

    if (JERRY_LIKELY (!context_p->token.lit_location.has_escape))
    {
      while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
      {
        if ((literal_p->type & SCANNER_LITERAL_IS_LOCAL)
            && !(literal_p->type & SCANNER_LITERAL_IS_ARG)
            && !((literal_p->type & SCANNER_LITERAL_IS_FUNC)
                 && (literal_pool_p->status_flags & SCANNER_LITERAL_POOL_FUNCTION))
            && (literal_p->type & SCANNER_LITERAL_IS_LOCAL) != SCANNER_LITERAL_IS_LOCAL
            && literal_p->length == length)
        {
          if (JERRY_LIKELY (!literal_p->has_escape))
          {
            if (memcmp (literal_p->char_p, char_p, length) == 0)
            {
              scanner_raise_redeclaration_error (context_p);
              return;
            }
          }
          else if (lexer_compare_identifier_to_string (literal_p, char_p, length))
          {
            scanner_raise_redeclaration_error (context_p);
            return;
          }
        }
      }
    }
    else
    {
      while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
      {
        if ((literal_p->type & SCANNER_LITERAL_IS_LOCAL)
            && !(literal_p->type & SCANNER_LITERAL_IS_ARG)
            && !((literal_p->type & SCANNER_LITERAL_IS_FUNC)
                 && (literal_pool_p->status_flags & SCANNER_LITERAL_POOL_FUNCTION))
            && (literal_p->type & SCANNER_LITERAL_IS_LOCAL) != SCANNER_LITERAL_IS_LOCAL
            && lexer_compare_identifiers (context_p, literal_p, var_literal_p))
        {
          scanner_raise_redeclaration_error (context_p);
          return;
        }
      }
    }
  }

  if (scanner_scope_find_lexical_declaration (context_p, var_literal_p))
  {
    scanner_raise_redeclaration_error (context_p);
  }
} /* scanner_detect_invalid_var */