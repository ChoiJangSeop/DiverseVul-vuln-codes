lexer_token_is_async (parser_context_t *context_p) /**< context */
{
  JERRY_ASSERT (context_p->token.type == LEXER_LITERAL
                || context_p->token.type == LEXER_TEMPLATE_LITERAL);

  return (context_p->token.keyword_type == LEXER_KEYW_ASYNC
          && !context_p->token.lit_location.has_escape);
} /* lexer_token_is_async */