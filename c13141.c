lexer_current_is_literal (parser_context_t *context_p, /**< context */
                          const lexer_lit_location_t *right_ident_p) /**< identifier */
{
  JERRY_ASSERT (context_p->token.type == LEXER_LITERAL
                && context_p->token.lit_location.type == LEXER_IDENT_LITERAL);

  lexer_lit_location_t *left_ident_p = &context_p->token.lit_location;

  JERRY_ASSERT (left_ident_p->length > 0 && right_ident_p->length > 0);

  if (left_ident_p->length != right_ident_p->length)
  {
    return false;
  }

  if (!left_ident_p->has_escape && !right_ident_p->has_escape)
  {
    return memcmp (left_ident_p->char_p, right_ident_p->char_p, left_ident_p->length) == 0;
  }

  return lexer_compare_identifiers (context_p, left_ident_p, right_ident_p);
} /* lexer_current_is_literal */