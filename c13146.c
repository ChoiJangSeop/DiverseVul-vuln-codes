lexer_token_is_let (parser_context_t *context_p) /**< context */
{
  JERRY_ASSERT (context_p->token.type == LEXER_LITERAL);

  return (context_p->token.keyword_type == LEXER_KEYW_LET
          && !context_p->token.lit_location.has_escape);
} /* lexer_token_is_let */