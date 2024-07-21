scanner_append_hole (parser_context_t *context_p, scanner_context_t *scanner_context_p)
{
  scanner_literal_pool_t *literal_pool_p = scanner_context_p->active_literal_pool_p;

  lexer_lit_location_t *literal_p;
  literal_p = (lexer_lit_location_t *) parser_list_append (context_p, &literal_pool_p->literal_pool);

  literal_p->char_p = NULL;
  literal_p->length = 0;
  literal_p->type = SCANNER_LITERAL_IS_ARG;
  literal_p->has_escape = 0;
} /* scanner_append_hole */