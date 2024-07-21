parser_parse_import_statement (parser_context_t *context_p) /**< parser context */
{
  JERRY_ASSERT (context_p->token.type == LEXER_KEYW_IMPORT);
  JERRY_ASSERT (context_p->module_names_p == NULL);

  if (lexer_check_next_character (context_p, LIT_CHAR_LEFT_PAREN))
  {
    if (context_p->status_flags & PARSER_IS_FUNCTION)
    {
      parser_parse_expression_statement (context_p, PARSE_EXPR);
      return;
    }

    parser_parse_block_expression (context_p, PARSE_EXPR);
    return;
  }

  parser_module_check_request_place (context_p);
  lexer_next_token (context_p);

  /* Check for a ModuleSpecifier*/
  if (context_p->token.type != LEXER_LITERAL
      || context_p->token.lit_location.type != LEXER_STRING_LITERAL)
  {
    if (!(context_p->token.type == LEXER_LEFT_BRACE
          || context_p->token.type == LEXER_MULTIPLY
          || (context_p->token.type == LEXER_LITERAL && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)))
    {
      parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_MULTIPLY_LITERAL_EXPECTED);
    }

    if (context_p->token.type == LEXER_LITERAL)
    {
      /* Handle ImportedDefaultBinding */
      lexer_construct_literal_object (context_p, &context_p->token.lit_location, LEXER_IDENT_LITERAL);

      ecma_string_t *local_name_p = ecma_new_ecma_string_from_utf8 (context_p->lit_object.literal_p->u.char_p,
                                                                    context_p->lit_object.literal_p->prop.length);

      if (parser_module_check_duplicate_import (context_p, local_name_p))
      {
        ecma_deref_ecma_string (local_name_p);
        parser_raise_error (context_p, PARSER_ERR_DUPLICATED_IMPORT_BINDING);
      }

      ecma_string_t *import_name_p = ecma_get_magic_string (LIT_MAGIC_STRING_DEFAULT);
      parser_module_add_names_to_node (context_p, import_name_p, local_name_p);

      ecma_deref_ecma_string (local_name_p);
      ecma_deref_ecma_string (import_name_p);

      lexer_next_token (context_p);

      if (context_p->token.type == LEXER_COMMA)
      {
        lexer_next_token (context_p);
        if (context_p->token.type != LEXER_MULTIPLY
            && context_p->token.type != LEXER_LEFT_BRACE)
        {
          parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_MULTIPLY_EXPECTED);
        }
      }
      else if (!lexer_token_is_identifier (context_p, "from", 4))
      {
        parser_raise_error (context_p, PARSER_ERR_FROM_COMMA_EXPECTED);
      }
    }

    if (context_p->token.type == LEXER_MULTIPLY)
    {
      /* NameSpaceImport */
      lexer_next_token (context_p);
      if (!lexer_token_is_identifier (context_p, "as", 2))
      {
        parser_raise_error (context_p, PARSER_ERR_AS_EXPECTED);
      }

      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_LITERAL)
      {
        parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
      }

      lexer_construct_literal_object (context_p, &context_p->token.lit_location, LEXER_IDENT_LITERAL);

      ecma_string_t *local_name_p = ecma_new_ecma_string_from_utf8 (context_p->lit_object.literal_p->u.char_p,
                                                                    context_p->lit_object.literal_p->prop.length);

      if (parser_module_check_duplicate_import (context_p, local_name_p))
      {
        ecma_deref_ecma_string (local_name_p);
        parser_raise_error (context_p, PARSER_ERR_DUPLICATED_IMPORT_BINDING);
      }

      ecma_string_t *import_name_p = ecma_get_magic_string (LIT_MAGIC_STRING_ASTERIX_CHAR);

      parser_module_add_names_to_node (context_p, import_name_p, local_name_p);
      ecma_deref_ecma_string (local_name_p);
      ecma_deref_ecma_string (import_name_p);

      lexer_next_token (context_p);
    }
    else if (context_p->token.type == LEXER_LEFT_BRACE)
    {
      /* Handle NamedImports */
      parser_module_parse_import_clause (context_p);
    }

    if (!lexer_token_is_identifier (context_p, "from", 4))
    {
      parser_raise_error (context_p, PARSER_ERR_FROM_EXPECTED);
    }
    lexer_next_token (context_p);
  }

  parser_module_handle_module_specifier (context_p, NULL);
} /* parser_parse_import_statement */