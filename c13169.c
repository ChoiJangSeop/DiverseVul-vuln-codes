parser_parse_export_statement (parser_context_t *context_p) /**< context */
{
  JERRY_ASSERT (context_p->token.type == LEXER_KEYW_EXPORT);
  JERRY_ASSERT (context_p->module_names_p == NULL);

  parser_module_check_request_place (context_p);

  bool consume_last_statement = false;

  lexer_next_token (context_p);
  switch (context_p->token.type)
  {
    case LEXER_KEYW_DEFAULT:
    {
      scanner_location_t location;
      scanner_get_location (&location, context_p);

      context_p->status_flags |= PARSER_MODULE_STORE_IDENT;

      lexer_next_token (context_p);

      if (context_p->token.type == LEXER_LITERAL
          && lexer_token_is_async (context_p)
          && context_p->next_scanner_info_p->source_p == context_p->source_p
          && context_p->next_scanner_info_p->type == SCANNER_TYPE_FUNCTION)
      {
        lexer_next_token (context_p);
      }

      if (context_p->token.type == LEXER_KEYW_CLASS)
      {
        context_p->status_flags |= PARSER_MODULE_DEFAULT_CLASS_OR_FUNC;
        parser_parse_class (context_p, true);
        consume_last_statement = true;
      }
      else if (context_p->token.type == LEXER_KEYW_FUNCTION)
      {
        context_p->status_flags |= PARSER_MODULE_DEFAULT_CLASS_OR_FUNC;
        parser_parse_function_statement (context_p);
        consume_last_statement = true;
      }
      else
      {
        /* Assignment expression */
        scanner_set_location (context_p, &location);

        /* 15.2.3.5 Use the synthetic name '*default*' as the identifier. */
        lexer_construct_literal_object (context_p, &lexer_default_literal, lexer_default_literal.type);

        context_p->token.lit_location.type = LEXER_IDENT_LITERAL;
        parser_emit_cbc_literal_from_token (context_p, CBC_PUSH_LITERAL);

        /* Do not overwrite this identifier. */
        context_p->status_flags &= (uint32_t) ~PARSER_MODULE_STORE_IDENT;
        context_p->module_identifier_lit_p = context_p->lit_object.literal_p;

        /* Fake an assignment to the default identifier */
        context_p->token.type = LEXER_ASSIGN;

        parser_parse_expression_statement (context_p, PARSE_EXPR_NO_COMMA | PARSE_EXPR_HAS_LITERAL);
      }

      ecma_string_t *name_p = ecma_new_ecma_string_from_utf8 (context_p->module_identifier_lit_p->u.char_p,
                                                              context_p->module_identifier_lit_p->prop.length);
      ecma_string_t *export_name_p = ecma_get_magic_string (LIT_MAGIC_STRING_DEFAULT);

      if (parser_module_check_duplicate_export (context_p, export_name_p))
      {
        ecma_deref_ecma_string (name_p);
        ecma_deref_ecma_string (export_name_p);
        parser_raise_error (context_p, PARSER_ERR_DUPLICATED_EXPORT_IDENTIFIER);
      }

      parser_module_add_names_to_node (context_p,
                                       export_name_p,
                                       name_p);
      ecma_deref_ecma_string (name_p);
      ecma_deref_ecma_string (export_name_p);
      break;
    }
    case LEXER_MULTIPLY:
    {
      lexer_next_token (context_p);

      ecma_module_node_t **target_node_list_p = &(JERRY_CONTEXT (module_current_p)->star_exports_p);

      if (lexer_token_is_identifier (context_p, "as", 2))
      {
        target_node_list_p = &(JERRY_CONTEXT (module_current_p)->indirect_exports_p);

        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_LITERAL
            || context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
        {
          parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
        }

        lexer_construct_literal_object (context_p, &context_p->token.lit_location, LEXER_NEW_IDENT_LITERAL);

        lexer_literal_t *literal_p = PARSER_GET_LITERAL (context_p->lit_object.index);
        ecma_string_t *export_name_p = ecma_new_ecma_string_from_utf8 (literal_p->u.char_p,
                                                                       literal_p->prop.length);

        if (parser_module_check_duplicate_export (context_p, export_name_p))
        {
          ecma_deref_ecma_string (export_name_p);
          parser_raise_error (context_p, PARSER_ERR_DUPLICATED_EXPORT_IDENTIFIER);
        }

        ecma_string_t *local_name_p = ecma_get_magic_string (LIT_MAGIC_STRING_ASTERIX_CHAR);
        parser_module_add_names_to_node (context_p, export_name_p, local_name_p);
        ecma_deref_ecma_string (export_name_p);

        lexer_next_token (context_p);
      }

      if (!lexer_token_is_identifier (context_p, "from", 4))
      {
        parser_raise_error (context_p, PARSER_ERR_FROM_EXPECTED);
      }

      lexer_next_token (context_p);
      parser_module_handle_module_specifier (context_p, target_node_list_p);
      return false;
    }
    case LEXER_KEYW_VAR:
    case LEXER_KEYW_LET:
    case LEXER_KEYW_CONST:
    {
      context_p->status_flags |= PARSER_MODULE_STORE_IDENT;
      parser_parse_var_statement (context_p);
      break;
    }
    case LEXER_KEYW_CLASS:
    {
      context_p->status_flags |= PARSER_MODULE_STORE_IDENT;
      parser_parse_class (context_p, true);
      consume_last_statement = true;
      break;
    }
    case LEXER_KEYW_FUNCTION:
    {
      context_p->status_flags |= PARSER_MODULE_STORE_IDENT;
      parser_parse_function_statement (context_p);
      consume_last_statement = true;
      break;
    }
    case LEXER_LEFT_BRACE:
    {
      parser_module_parse_export_clause (context_p);

      if (lexer_token_is_identifier (context_p, "from", 4))
      {
        lexer_next_token (context_p);
        parser_module_handle_module_specifier (context_p, &(JERRY_CONTEXT (module_current_p)->indirect_exports_p));
        return false;
      }
      break;
    }
    default:
    {
      parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_MULTIPLY_LITERAL_EXPECTED);
      break;
    }
  }

  context_p->status_flags &= (uint32_t) ~(PARSER_MODULE_DEFAULT_CLASS_OR_FUNC | PARSER_MODULE_STORE_IDENT);
  parser_module_append_names (context_p, &(JERRY_CONTEXT (module_current_p)->local_exports_p));

  return consume_last_statement;
} /* parser_parse_export_statement */