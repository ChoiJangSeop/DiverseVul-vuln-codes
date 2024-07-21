parser_parse_for_statement_start (parser_context_t *context_p) /**< context */
{
  parser_loop_statement_t loop;

  JERRY_ASSERT (context_p->token.type == LEXER_KEYW_FOR);
  lexer_next_token (context_p);

#if JERRY_ESNEXT
  bool is_for_await = false;

  if (context_p->token.type == LEXER_KEYW_AWAIT)
  {
    if (JERRY_UNLIKELY (context_p->token.lit_location.status_flags & LEXER_LIT_LOCATION_HAS_ESCAPE))
    {
      parser_raise_error (context_p, PARSER_ERR_INVALID_KEYWORD);
    }
    lexer_next_token (context_p);
    is_for_await = true;
  }
#endif /* JERRY_ESNEXT */

  if (context_p->token.type != LEXER_LEFT_PAREN)
  {
#if JERRY_ESNEXT
    if (context_p->token.type == LEXER_LITERAL
        && context_p->token.keyword_type == LEXER_KEYW_AWAIT
        && !(context_p->token.lit_location.status_flags & LEXER_LIT_LOCATION_HAS_ESCAPE))
    {
      parser_raise_error (context_p, PARSER_ERR_FOR_AWAIT_NO_ASYNC);
    }
#endif /* JERRY_ESNEXT */
    parser_raise_error (context_p, PARSER_ERR_LEFT_PAREN_EXPECTED);
  }

  if (context_p->next_scanner_info_p->source_p == context_p->source_p)
  {
    parser_for_in_of_statement_t for_in_of_statement;
    scanner_location_t start_location, end_location;

#if JERRY_ESNEXT
    JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_FOR_IN
                  || context_p->next_scanner_info_p->type == SCANNER_TYPE_FOR_OF);

    bool is_for_in = (context_p->next_scanner_info_p->type == SCANNER_TYPE_FOR_IN);
    end_location = ((scanner_location_info_t *) context_p->next_scanner_info_p)->location;

    scanner_release_next (context_p, sizeof (scanner_location_info_t));

    scanner_get_location (&start_location, context_p);
    lexer_next_token (context_p);

    uint8_t token_type = LEXER_EOS;
    bool has_context = false;

    if (context_p->token.type == LEXER_KEYW_VAR
        || context_p->token.type == LEXER_KEYW_LET
        || context_p->token.type == LEXER_KEYW_CONST)
    {
      token_type = context_p->token.type;
      has_context = context_p->next_scanner_info_p->source_p == context_p->source_p;
      JERRY_ASSERT (!has_context || context_p->next_scanner_info_p->type == SCANNER_TYPE_BLOCK);
      scanner_get_location (&start_location, context_p);

      /* TODO: remove this after the pre-scanner supports strict mode detection. */
      if (context_p->next_scanner_info_p->source_p == context_p->source_p
          && context_p->next_scanner_info_p->type == SCANNER_TYPE_LET_EXPRESSION)
      {
        scanner_release_next (context_p, sizeof (scanner_info_t));
      }
    }
    else if (context_p->token.type == LEXER_LITERAL && lexer_token_is_let (context_p))
    {
      if (context_p->next_scanner_info_p->source_p == context_p->source_p
          && context_p->next_scanner_info_p->type == SCANNER_TYPE_LET_EXPRESSION)
      {
        scanner_release_next (context_p, sizeof (scanner_info_t));
      }
      else
      {
        token_type = LEXER_KEYW_LET;
        has_context = (context_p->next_scanner_info_p->source_p == context_p->source_p);
        scanner_get_location (&start_location, context_p);
      }
    }

    if (has_context)
    {
      has_context = parser_push_block_context (context_p, true);
    }

    scanner_set_location (context_p, &end_location);
#else /* !JERRY_ESNEXT */
    JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_FOR_IN);

    bool is_for_in = true;
    scanner_get_location (&start_location, context_p);

    scanner_set_location (context_p, &((scanner_location_info_t *) context_p->next_scanner_info_p)->location);
    scanner_release_next (context_p, sizeof (scanner_location_info_t));
#endif /* JERRY_ESNEXT */

    /* The length of both 'in' and 'of' is two. */
    const uint8_t *source_end_p = context_p->source_p - 2;

    scanner_seek (context_p);

#if JERRY_ESNEXT
    if (is_for_in && is_for_await)
    {
      context_p->token.line = context_p->line;
      context_p->token.column = context_p->column - 2;
      parser_raise_error (context_p, PARSER_ERR_FOR_AWAIT_NO_OF);
    }
#endif /* JERRY_ESNEXT */

    lexer_next_token (context_p);
    int options = is_for_in ? PARSE_EXPR : PARSE_EXPR_LEFT_HAND_SIDE;
    parser_parse_expression (context_p, options);

    if (context_p->token.type != LEXER_RIGHT_PAREN)
    {
      parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
    }

#ifndef JERRY_NDEBUG
    PARSER_PLUS_EQUAL_U16 (context_p->context_stack_depth,
                           is_for_in ? PARSER_FOR_IN_CONTEXT_STACK_ALLOCATION
                                     : PARSER_FOR_OF_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

    cbc_ext_opcode_t init_opcode = CBC_EXT_FOR_IN_INIT;

#if JERRY_ESNEXT
    if (!is_for_in)
    {
      init_opcode = is_for_await ? CBC_EXT_FOR_AWAIT_OF_INIT : CBC_EXT_FOR_OF_INIT;
    }
#endif /* JERRY_ESNEXT */

    parser_emit_cbc_ext_forward_branch (context_p, init_opcode, &for_in_of_statement.branch);

    JERRY_ASSERT (context_p->last_cbc_opcode == PARSER_CBC_UNAVAILABLE);
    for_in_of_statement.start_offset = context_p->byte_code_size;

#if JERRY_ESNEXT
    if (has_context)
    {
      parser_emit_cbc_ext (context_p, CBC_EXT_CLONE_CONTEXT);
    }
#endif /* JERRY_ESNEXT */

    /* The expression parser must not read the 'in' or 'of' tokens. */
    scanner_get_location (&end_location, context_p);
    scanner_set_location (context_p, &start_location);

    const uint8_t *original_source_end_p = context_p->source_end_p;
    context_p->source_end_p = source_end_p;
    scanner_seek (context_p);

#if JERRY_ESNEXT
    if (token_type == LEXER_EOS)
    {
      lexer_next_token (context_p);

      if (context_p->token.type == LEXER_LEFT_SQUARE || context_p->token.type == LEXER_LEFT_BRACE)
      {
        token_type = context_p->token.type;
      }
    }
#else /* !JERRY_ESNEXT */
    lexer_next_token (context_p);

    uint8_t token_type = context_p->token.type;
#endif /* JERRY_ESNEXT */

    switch (token_type)
    {
#if JERRY_ESNEXT
      case LEXER_KEYW_LET:
      case LEXER_KEYW_CONST:
#endif /* JERRY_ESNEXT */
      case LEXER_KEYW_VAR:
      {
#if JERRY_ESNEXT
        if (lexer_check_next_characters (context_p, LIT_CHAR_LEFT_SQUARE, LIT_CHAR_LEFT_BRACE))
        {
          parser_emit_cbc_ext (context_p, is_for_in ? CBC_EXT_FOR_IN_GET_NEXT
                                                    : CBC_EXT_FOR_OF_GET_NEXT);

          parser_pattern_flags_t flags = (PARSER_PATTERN_BINDING | PARSER_PATTERN_TARGET_ON_STACK);

          if (context_p->next_scanner_info_p->source_p == (context_p->source_p + 1))
          {
            if (context_p->next_scanner_info_p->type == SCANNER_TYPE_INITIALIZER)
            {
              scanner_release_next (context_p, sizeof (scanner_location_info_t));
            }
            else
            {
              JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_LITERAL_FLAGS);
              if (context_p->next_scanner_info_p->u8_arg & SCANNER_LITERAL_OBJECT_HAS_REST)
              {
                flags |= PARSER_PATTERN_HAS_REST_ELEMENT;
              }

              scanner_release_next (context_p, sizeof (scanner_info_t));
            }
          }

          if (token_type == LEXER_KEYW_LET)
          {
            flags |= PARSER_PATTERN_LET;
          }
          else if (token_type == LEXER_KEYW_CONST)
          {
            flags |= PARSER_PATTERN_CONST;
          }

          parser_parse_initializer_by_next_char (context_p, flags);
          break;
        }
#endif /* JERRY_ESNEXT */

        lexer_expect_identifier (context_p, LEXER_IDENT_LITERAL);

#if JERRY_ESNEXT
        if (context_p->token.keyword_type == LEXER_KEYW_LET
            && token_type != LEXER_KEYW_VAR)
        {
          parser_raise_error (context_p, PARSER_ERR_LEXICAL_LET_BINDING);
        }
#endif /* JERRY_ESNEXT */

        JERRY_ASSERT (context_p->token.type == LEXER_LITERAL
                      && context_p->token.lit_location.type == LEXER_IDENT_LITERAL);

        uint16_t literal_index = context_p->lit_object.index;
        lexer_next_token (context_p);

        if (context_p->token.type == LEXER_ASSIGN)
        {
#if JERRY_ESNEXT
          if ((context_p->status_flags & PARSER_IS_STRICT) || !is_for_in)
          {
            parser_raise_error (context_p, PARSER_ERR_FOR_IN_OF_DECLARATION);
          }
#endif /* JERRY_ESNEXT */
          parser_branch_t branch;

          /* Initialiser is never executed. */
          parser_emit_cbc_forward_branch (context_p, CBC_JUMP_FORWARD, &branch);
          lexer_next_token (context_p);
          parser_parse_expression_statement (context_p, PARSE_EXPR_NO_COMMA);
          parser_set_branch_to_current_position (context_p, &branch);
        }

        parser_emit_cbc_ext (context_p, is_for_in ? CBC_EXT_FOR_IN_GET_NEXT
                                                  : CBC_EXT_FOR_OF_GET_NEXT);
#if JERRY_ESNEXT
#ifndef JERRY_NDEBUG
        if (literal_index < PARSER_REGISTER_START
            && has_context
            && !scanner_literal_is_created (context_p, literal_index))
        {
          context_p->global_status_flags |= ECMA_PARSE_INTERNAL_FOR_IN_OFF_CONTEXT_ERROR;
        }
#endif /* !JERRY_NDEBUG */

        uint16_t opcode = (has_context ? CBC_ASSIGN_LET_CONST : CBC_ASSIGN_SET_IDENT);
        parser_emit_cbc_literal (context_p, opcode, literal_index);
#else /* !JERRY_ESNEXT */
        parser_emit_cbc_literal (context_p, CBC_ASSIGN_SET_IDENT, literal_index);
#endif /* JERRY_ESNEXT */
        break;
      }
#if JERRY_ESNEXT
      case LEXER_LEFT_BRACE:
      case LEXER_LEFT_SQUARE:
      {
        if (context_p->next_scanner_info_p->source_p == context_p->source_p
            && context_p->next_scanner_info_p->type == SCANNER_TYPE_LITERAL_FLAGS
            && (context_p->next_scanner_info_p->u8_arg & SCANNER_LITERAL_DESTRUCTURING_FOR))
        {
          parser_emit_cbc_ext (context_p, is_for_in ? CBC_EXT_FOR_IN_GET_NEXT
                                                    : CBC_EXT_FOR_OF_GET_NEXT);

          uint32_t flags = PARSER_PATTERN_TARGET_ON_STACK;

          if (context_p->next_scanner_info_p->u8_arg & SCANNER_LITERAL_OBJECT_HAS_REST)
          {
            flags |= PARSER_PATTERN_HAS_REST_ELEMENT;
          }

          scanner_release_next (context_p, sizeof (scanner_info_t));
          parser_parse_initializer (context_p, flags);
          /* Pop the value returned by GET_NEXT. */
          parser_emit_cbc (context_p, CBC_POP);
          break;
        }
        /* FALLTHRU */
      }
#endif /* JERRY_ESNEXT */
      default:
      {
        uint16_t opcode;

        parser_parse_expression (context_p, PARSE_EXPR_LEFT_HAND_SIDE);

        opcode = context_p->last_cbc_opcode;

        /* The CBC_EXT_FOR_IN_CREATE_CONTEXT flushed the opcode combiner. */
        JERRY_ASSERT (opcode != CBC_PUSH_TWO_LITERALS
                      && opcode != CBC_PUSH_THREE_LITERALS);

        opcode = parser_check_left_hand_side_expression (context_p, opcode);

        parser_emit_cbc_ext (context_p, is_for_in ? CBC_EXT_FOR_IN_GET_NEXT
                                                  : CBC_EXT_FOR_OF_GET_NEXT);
        parser_flush_cbc (context_p);

        context_p->last_cbc_opcode = opcode;
        break;
      }
    }

    if (context_p->token.type != LEXER_EOS)
    {
#if JERRY_ESNEXT
      parser_raise_error (context_p, is_for_in ? PARSER_ERR_IN_EXPECTED : PARSER_ERR_OF_EXPECTED);
#else /* !JERRY_ESNEXT */
      parser_raise_error (context_p, PARSER_ERR_IN_EXPECTED);
#endif /* JERRY_ESNEXT */
    }

    parser_flush_cbc (context_p);
    scanner_set_location (context_p, &end_location);
    context_p->source_end_p = original_source_end_p;
    lexer_next_token (context_p);

    loop.branch_list_p = NULL;

    parser_stack_push (context_p, &for_in_of_statement, sizeof (parser_for_in_of_statement_t));
    parser_stack_push (context_p, &loop, sizeof (parser_loop_statement_t));

    uint8_t for_type = PARSER_STATEMENT_FOR_IN;

#if JERRY_ESNEXT
    if (!is_for_in)
    {
      for_type = is_for_await ? PARSER_STATEMENT_FOR_AWAIT_OF : PARSER_STATEMENT_FOR_OF;
    }
#endif /* JERRY_ESNEXT */

    parser_stack_push_uint8 (context_p, for_type);
    parser_stack_iterator_init (context_p, &context_p->last_statement);
    return;
  }

  lexer_next_token (context_p);

  if (context_p->token.type != LEXER_SEMICOLON)
  {
#if JERRY_ESNEXT
    const uint8_t *source_p = context_p->source_p;
#endif /* JERRY_ESNEXT */

    switch (context_p->token.type)
    {
#if JERRY_ESNEXT
      case LEXER_LITERAL:
      {
        if (!lexer_token_is_let (context_p))
        {
          parser_parse_expression_statement (context_p, PARSE_EXPR);
          break;
        }

        if (context_p->next_scanner_info_p->source_p == context_p->source_p
            && context_p->next_scanner_info_p->type != SCANNER_TYPE_BLOCK)
        {
          if (context_p->next_scanner_info_p->type == SCANNER_TYPE_LET_EXPRESSION)
          {
            scanner_release_next (context_p, sizeof (scanner_info_t));
          }

          parser_parse_expression_statement (context_p, PARSE_EXPR);
          break;
        }

        context_p->token.type = LEXER_KEYW_LET;
        /* FALLTHRU */
      }
      case LEXER_KEYW_LET:
      case LEXER_KEYW_CONST:
      {
        if (context_p->next_scanner_info_p->source_p == source_p)
        {
          parser_push_block_context (context_p, true);
        }
        /* FALLTHRU */
      }
#endif /* JERRY_ESNEXT */
      case LEXER_KEYW_VAR:
      {
        parser_parse_var_statement (context_p);
        break;
      }
      default:
      {
        parser_parse_expression_statement (context_p, PARSE_EXPR);
        break;
      }
    }

    if (context_p->token.type != LEXER_SEMICOLON)
    {
      parser_raise_error (context_p, PARSER_ERR_SEMICOLON_EXPECTED);
    }
  }

#if JERRY_ESNEXT
  if (is_for_await)
  {
    parser_raise_error (context_p, PARSER_ERR_FOR_AWAIT_NO_OF);
  }
#endif /* JERRY_ESNEXT */

  JERRY_ASSERT (context_p->next_scanner_info_p->source_p != context_p->source_p
                || context_p->next_scanner_info_p->type == SCANNER_TYPE_FOR);

  if (context_p->next_scanner_info_p->source_p != context_p->source_p
      || ((scanner_for_info_t *) context_p->next_scanner_info_p)->end_location.source_p == NULL)
  {
    if (context_p->next_scanner_info_p->source_p == context_p->source_p)
    {
      /* Even though the scanning is failed, there might be valid statements
       * inside the for statement which depend on scanner info blocks. */
      scanner_release_next (context_p, sizeof (scanner_for_info_t));
    }

    /* The prescanner couldn't find the second semicolon or the closing paranthesis. */
    lexer_next_token (context_p);
    parser_parse_expression (context_p, PARSE_EXPR);

    if (context_p->token.type != LEXER_SEMICOLON)
    {
      parser_raise_error (context_p, PARSER_ERR_SEMICOLON_EXPECTED);
    }

    lexer_next_token (context_p);
    parser_parse_expression_statement (context_p, PARSE_EXPR);

    JERRY_ASSERT (context_p->token.type != LEXER_RIGHT_PAREN);
    parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
  }

  parser_for_statement_t for_statement;
  scanner_for_info_t *for_info_p = (scanner_for_info_t *) context_p->next_scanner_info_p;

  parser_emit_cbc_forward_branch (context_p, CBC_JUMP_FORWARD, &for_statement.branch);

  JERRY_ASSERT (context_p->last_cbc_opcode == PARSER_CBC_UNAVAILABLE);

  for_statement.start_offset = context_p->byte_code_size;
  scanner_get_location (&for_statement.condition_location, context_p);
  for_statement.expression_location = for_info_p->expression_location;

  scanner_set_location (context_p, &for_info_p->end_location);
  scanner_release_next (context_p, sizeof (scanner_for_info_t));
  scanner_seek (context_p);
  lexer_next_token (context_p);

  loop.branch_list_p = NULL;

  parser_stack_push (context_p, &for_statement, sizeof (parser_for_statement_t));
  parser_stack_push (context_p, &loop, sizeof (parser_loop_statement_t));
  parser_stack_push_uint8 (context_p, PARSER_STATEMENT_FOR);
  parser_stack_iterator_init (context_p, &context_p->last_statement);
} /* parser_parse_for_statement_start */