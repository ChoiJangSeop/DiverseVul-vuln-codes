lexer_expect_object_literal_id (parser_context_t *context_p, /**< context */
                                uint32_t ident_opts) /**< lexer_obj_ident_opts_t option bits */
{
  lexer_skip_spaces (context_p);

  if (context_p->source_p >= context_p->source_end_p)
  {
    parser_raise_error (context_p, PARSER_ERR_PROPERTY_IDENTIFIER_EXPECTED);
  }

  context_p->token.keyword_type = LEXER_EOS;
  context_p->token.line = context_p->line;
  context_p->token.column = context_p->column;
  bool create_literal_object = false;

  JERRY_ASSERT ((ident_opts & LEXER_OBJ_IDENT_CLASS_IDENTIFIER) || !(ident_opts & LEXER_OBJ_IDENT_CLASS_NO_STATIC));

#if JERRY_FUNCTION_TO_STRING
  if (ident_opts & LEXER_OBJ_IDENT_SET_FUNCTION_START)
  {
    context_p->function_start_p = context_p->source_p;
  }
#endif /* JERRY_FUNCTION_TO_STRING */

  if (lexer_parse_identifier (context_p, LEXER_PARSE_NO_OPTS))
  {
    if (!(ident_opts & (LEXER_OBJ_IDENT_ONLY_IDENTIFIERS | LEXER_OBJ_IDENT_OBJECT_PATTERN)))
    {
      lexer_skip_spaces (context_p);
      context_p->token.flags = (uint8_t) (context_p->token.flags | LEXER_NO_SKIP_SPACES);

      if (context_p->source_p < context_p->source_end_p
#if JERRY_ESNEXT
          && context_p->source_p[0] != LIT_CHAR_COMMA && context_p->source_p[0] != LIT_CHAR_RIGHT_BRACE
          && context_p->source_p[0] != LIT_CHAR_LEFT_PAREN && context_p->source_p[0] != LIT_CHAR_SEMICOLON
          && context_p->source_p[0] != LIT_CHAR_EQUALS
#endif /* JERRY_ESNEXT */
          && context_p->source_p[0] != LIT_CHAR_COLON)
      {
        if (lexer_compare_literal_to_string (context_p, "get", 3))
        {
          context_p->token.type = LEXER_PROPERTY_GETTER;
          return;
        }

        if (lexer_compare_literal_to_string (context_p, "set", 3))
        {
          context_p->token.type = LEXER_PROPERTY_SETTER;
          return;
        }

#if JERRY_ESNEXT
        if (lexer_compare_literal_to_string (context_p, "async", 5))
        {
          context_p->token.type = LEXER_KEYW_ASYNC;
          return;
        }

        if (ident_opts & LEXER_OBJ_IDENT_CLASS_NO_STATIC)
        {
          if (lexer_compare_literal_to_string (context_p, "static", 6))
          {
            context_p->token.type = LEXER_KEYW_STATIC;
          }
          return;
        }
#endif /* JERRY_ESNEXT */
      }
    }

    create_literal_object = true;
  }
#if JERRY_ESNEXT
  else if (ident_opts & LEXER_OBJ_IDENT_CLASS_PRIVATE)
  {
    parser_raise_error (context_p, PARSER_ERR_INVALID_CHARACTER);
  }
#endif /* JERRY_ESNEXT */
  else
  {
    switch (context_p->source_p[0])
    {
      case LIT_CHAR_DOUBLE_QUOTE:
      case LIT_CHAR_SINGLE_QUOTE:
      {
        lexer_parse_string (context_p, LEXER_STRING_NO_OPTS);
        create_literal_object = true;
        break;
      }
#if JERRY_ESNEXT
      case LIT_CHAR_LEFT_SQUARE:
      {
#if JERRY_FUNCTION_TO_STRING
        const uint8_t *function_start_p = context_p->function_start_p;
#endif /* JERRY_FUNCTION_TO_STRING */

        lexer_consume_next_character (context_p);

        lexer_next_token (context_p);
        parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);

        if (context_p->token.type != LEXER_RIGHT_SQUARE)
        {
          parser_raise_error (context_p, PARSER_ERR_RIGHT_SQUARE_EXPECTED);
        }

#if JERRY_FUNCTION_TO_STRING
        context_p->function_start_p = function_start_p;
#endif /* JERRY_FUNCTION_TO_STRING */
        return;
      }
      case LIT_CHAR_ASTERISK:
      {
        if (ident_opts & (LEXER_OBJ_IDENT_ONLY_IDENTIFIERS | LEXER_OBJ_IDENT_OBJECT_PATTERN))
        {
          break;
        }

        context_p->token.type = LEXER_MULTIPLY;
        lexer_consume_next_character (context_p);
        return;
      }
      case LIT_CHAR_HASHMARK:
      {
        if (ident_opts & LEXER_OBJ_IDENT_CLASS_IDENTIFIER)
        {
          context_p->token.type = LEXER_HASHMARK;
          return;
        }

        break;
      }
#endif /* JERRY_ESNEXT */
      case LIT_CHAR_LEFT_BRACE:
      {
        if (ident_opts & (LEXER_OBJ_IDENT_CLASS_NO_STATIC | LEXER_OBJ_IDENT_CLASS_PRIVATE))
        {
          break;
        }

        context_p->token.type = LEXER_LEFT_BRACE;
        lexer_consume_next_character (context_p);
        return;
      }
      case LIT_CHAR_RIGHT_BRACE:
      {
        if (ident_opts & LEXER_OBJ_IDENT_ONLY_IDENTIFIERS)
        {
          break;
        }

        context_p->token.type = LEXER_RIGHT_BRACE;
        lexer_consume_next_character (context_p);
        return;
      }
#if JERRY_ESNEXT
      case LIT_CHAR_DOT:
      {
        if (!(context_p->source_p + 1 >= context_p->source_end_p || lit_char_is_decimal_digit (context_p->source_p[1])))
        {
          if ((ident_opts & ((uint32_t) ~(LEXER_OBJ_IDENT_OBJECT_PATTERN | LEXER_OBJ_IDENT_SET_FUNCTION_START)))
              || context_p->source_p + 2 >= context_p->source_end_p || context_p->source_p[1] != LIT_CHAR_DOT
              || context_p->source_p[2] != LIT_CHAR_DOT)
          {
            break;
          }

          context_p->token.type = LEXER_THREE_DOTS;
          context_p->token.flags &= (uint8_t) ~LEXER_NO_SKIP_SPACES;
          PARSER_PLUS_EQUAL_LC (context_p->column, 3);
          context_p->source_p += 3;
          return;
        }
        /* FALLTHRU */
      }
#endif /* JERRY_ESNEXT */
      default:
      {
        const uint8_t *char_p = context_p->source_p;

        if (char_p[0] == LIT_CHAR_DOT)
        {
          char_p++;
        }

        if (char_p < context_p->source_end_p && char_p[0] >= LIT_CHAR_0 && char_p[0] <= LIT_CHAR_9)
        {
          lexer_parse_number (context_p);

          if (!(ident_opts & LEXER_OBJ_IDENT_CLASS_IDENTIFIER))
          {
            lexer_construct_number_object (context_p, false, false);
          }
          return;
        }
        break;
      }
    }
  }

  if (create_literal_object)
  {
#if JERRY_ESNEXT
    if (ident_opts & LEXER_OBJ_IDENT_CLASS_IDENTIFIER)
    {
      return;
    }

    if (ident_opts & LEXER_OBJ_IDENT_CLASS_PRIVATE)
    {
      parser_resolve_private_identifier (context_p);
      return;
    }
#endif /* JERRY_ESNEXT */

    lexer_construct_literal_object (context_p, &context_p->token.lit_location, LEXER_STRING_LITERAL);
    return;
  }

  parser_raise_error (context_p, PARSER_ERR_PROPERTY_IDENTIFIER_EXPECTED);
} /* lexer_expect_object_literal_id */