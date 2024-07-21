parser_module_parse_export_clause (parser_context_t *context_p) /**< parser context */
{
  bool has_module_specifier = false;

  if (context_p->source_p == context_p->next_scanner_info_p->source_p)
  {
    has_module_specifier = true;
    JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_EXPORT_MODULE_SPECIFIER);
    scanner_release_next (context_p, sizeof (scanner_info_t));
  }

  JERRY_ASSERT (context_p->token.type == LEXER_LEFT_BRACE);
  lexer_next_token (context_p);

  while (true)
  {
    if (context_p->token.type == LEXER_RIGHT_BRACE)
    {
      lexer_next_token (context_p);
      break;
    }

    /* 15.2.3.1 The referenced binding cannot be a reserved word. */
    if (context_p->token.type != LEXER_LITERAL
        || context_p->token.lit_location.type != LEXER_IDENT_LITERAL
        || context_p->token.keyword_type >= LEXER_FIRST_FUTURE_STRICT_RESERVED_WORD)
    {
      parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
    }

    ecma_string_t *export_name_p = NULL;
    ecma_string_t *local_name_p = NULL;

    lexer_construct_literal_object (context_p, &context_p->token.lit_location, LEXER_NEW_IDENT_LITERAL);

    if (!has_module_specifier
        && !scanner_literal_exists (context_p, context_p->lit_object.index))
    {
      parser_raise_error (context_p, PARSER_ERR_EXPORT_NOT_DEFINED);
    }

    uint16_t local_name_index = context_p->lit_object.index;
    uint16_t export_name_index = PARSER_MAXIMUM_NUMBER_OF_LITERALS;

    lexer_next_token (context_p);
    if (lexer_token_is_identifier (context_p, "as", 2))
    {
      lexer_next_token (context_p);

      if (context_p->token.type != LEXER_LITERAL
          || context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
      {
        parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
      }

      lexer_construct_literal_object (context_p, &context_p->token.lit_location, LEXER_NEW_IDENT_LITERAL);

      export_name_index = context_p->lit_object.index;

      lexer_next_token (context_p);
    }

    lexer_literal_t *literal_p = PARSER_GET_LITERAL (local_name_index);
    local_name_p = ecma_new_ecma_string_from_utf8 (literal_p->u.char_p, literal_p->prop.length);

    if (export_name_index != PARSER_MAXIMUM_NUMBER_OF_LITERALS)
    {
      lexer_literal_t *as_literal_p = PARSER_GET_LITERAL (export_name_index);
      export_name_p = ecma_new_ecma_string_from_utf8 (as_literal_p->u.char_p, as_literal_p->prop.length);
    }
    else
    {
      export_name_p = local_name_p;
      ecma_ref_ecma_string (local_name_p);
    }

    if (parser_module_check_duplicate_export (context_p, export_name_p))
    {
      ecma_deref_ecma_string (local_name_p);
      ecma_deref_ecma_string (export_name_p);
      parser_raise_error (context_p, PARSER_ERR_DUPLICATED_EXPORT_IDENTIFIER);
    }

    parser_module_add_names_to_node (context_p, export_name_p, local_name_p);
    ecma_deref_ecma_string (local_name_p);
    ecma_deref_ecma_string (export_name_p);

    if (context_p->token.type != LEXER_COMMA
        && context_p->token.type != LEXER_RIGHT_BRACE)
    {
      parser_raise_error (context_p, PARSER_ERR_RIGHT_BRACE_COMMA_EXPECTED);
    }
    else if (context_p->token.type == LEXER_COMMA)
    {
      lexer_next_token (context_p);
    }

    if (lexer_token_is_identifier (context_p, "from", 4))
    {
      parser_raise_error (context_p, PARSER_ERR_RIGHT_BRACE_EXPECTED);
    }
  }
} /* parser_module_parse_export_clause */