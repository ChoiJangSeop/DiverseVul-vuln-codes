parser_parse_unary_expression (parser_context_t *context_p, /**< context */
                               size_t *grouping_level_p) /**< grouping level */
{
  bool new_was_seen = false;

  /* Collect unary operators. */
  while (true)
  {
    /* Convert plus and minus binary operators to unary operators. */
    switch (context_p->token.type)
    {
      case LEXER_ADD:
      {
        context_p->token.type = LEXER_PLUS;
        break;
      }
      case LEXER_SUBTRACT:
      {
        context_p->token.type = LEXER_NEGATE;
        break;
      }
#if JERRY_ESNEXT
      case LEXER_KEYW_AWAIT:
      {
#if JERRY_MODULE_SYSTEM
        if ((context_p->global_status_flags & ECMA_PARSE_MODULE)
            && !(context_p->status_flags & PARSER_IS_ASYNC_FUNCTION))
        {
          parser_raise_error (context_p, PARSER_ERR_AWAIT_NOT_ALLOWED);
        }
#endif /* JERRY_MODULE_SYSTEM */

        if (JERRY_UNLIKELY (context_p->token.lit_location.has_escape))
        {
          parser_raise_error (context_p, PARSER_ERR_INVALID_KEYWORD);
        }
        break;
      }
#endif /* JERRY_ESNEXT */
#if JERRY_MODULE_SYSTEM
      case LEXER_KEYW_IMPORT:
      {
        if (new_was_seen)
        {
          parser_raise_error (context_p, PARSER_ERR_IMPORT_AFTER_NEW);
        }
        break;
      }
#endif /* JERRY_MODULE_SYSTEM */
    }

    /* Bracketed expressions are primary expressions. At this
     * point their left paren is pushed onto the stack and
     * they are processed when their closing paren is reached. */
    if (context_p->token.type == LEXER_LEFT_PAREN)
    {
#if JERRY_ESNEXT
      if (context_p->next_scanner_info_p->source_p == context_p->source_p)
      {
        JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_FUNCTION);
        break;
      }
#endif /* JERRY_ESNEXT */
      (*grouping_level_p) += PARSER_GROUPING_LEVEL_INCREASE;
      new_was_seen = false;
    }
    else if (context_p->token.type == LEXER_KEYW_NEW)
    {
      /* After 'new' unary operators are not allowed. */
      new_was_seen = true;

#if JERRY_ESNEXT
      /* Check if "new.target" is written here. */
      if (scanner_try_scan_new_target (context_p))
      {
        if (!(context_p->status_flags & PARSER_ALLOW_NEW_TARGET))
        {
          parser_raise_error (context_p, PARSER_ERR_NEW_TARGET_NOT_ALLOWED);
        }

        parser_emit_cbc_ext (context_p, CBC_EXT_PUSH_NEW_TARGET);
        lexer_next_token (context_p);
        /* Found "new.target" return here */
        return false;
      }
#endif /* JERRY_ESNEXT */
    }
    else if (new_was_seen
             || (*grouping_level_p == PARSE_EXPR_LEFT_HAND_SIDE)
             || !LEXER_IS_UNARY_OP_TOKEN (context_p->token.type))
    {
      break;
    }

    parser_stack_push_uint8 (context_p, context_p->token.type);
    lexer_next_token (context_p);
  }

  /* Parse primary expression. */
  switch (context_p->token.type)
  {
#if JERRY_ESNEXT
    case LEXER_TEMPLATE_LITERAL:
    {
      if (context_p->source_p[-1] != LIT_CHAR_GRAVE_ACCENT)
      {
        parser_parse_template_literal (context_p);
        break;
      }

      /* The string is a normal string literal. */
      /* FALLTHRU */
    }
#endif /* JERRY_ESNEXT */
    case LEXER_LITERAL:
    {
#if JERRY_ESNEXT
      if (JERRY_UNLIKELY (context_p->next_scanner_info_p->source_p == context_p->source_p))
      {
        JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_FUNCTION);

        uint32_t arrow_status_flags = (PARSER_IS_FUNCTION
                                       | PARSER_IS_ARROW_FUNCTION
                                       | (context_p->status_flags & PARSER_INSIDE_CLASS_FIELD));

        if (context_p->next_scanner_info_p->u8_arg & SCANNER_FUNCTION_ASYNC)
        {
          JERRY_ASSERT (lexer_token_is_async (context_p));
          JERRY_ASSERT (!(context_p->next_scanner_info_p->u8_arg & SCANNER_FUNCTION_STATEMENT));

          uint32_t saved_status_flags = context_p->status_flags;

          context_p->status_flags |= PARSER_IS_ASYNC_FUNCTION | PARSER_DISALLOW_AWAIT_YIELD;
          lexer_next_token (context_p);
          context_p->status_flags = saved_status_flags;

          if (context_p->token.type == LEXER_KEYW_FUNCTION)
          {
            uint32_t status_flags = (PARSER_FUNCTION_CLOSURE
                                     | PARSER_IS_FUNC_EXPRESSION
                                     | PARSER_IS_ASYNC_FUNCTION
                                     | PARSER_DISALLOW_AWAIT_YIELD);
            parser_parse_function_expression (context_p, status_flags);
            break;
          }

          arrow_status_flags = (PARSER_IS_FUNCTION
                                | PARSER_IS_ARROW_FUNCTION
                                | PARSER_IS_ASYNC_FUNCTION
                                | PARSER_DISALLOW_AWAIT_YIELD);
        }

        parser_check_assignment_expr (context_p);
        parser_parse_function_expression (context_p, arrow_status_flags);
        return parser_abort_parsing_after_assignment_expression (context_p);
      }
#endif /* JERRY_ESNEXT */

      uint8_t type = context_p->token.lit_location.type;

      if (type == LEXER_IDENT_LITERAL || type == LEXER_STRING_LITERAL)
      {
        lexer_construct_literal_object (context_p,
                                        &context_p->token.lit_location,
                                        context_p->token.lit_location.type);
      }
      else if (type == LEXER_NUMBER_LITERAL)
      {
        bool is_negative_number = false;

        if ((context_p->stack_top_uint8 == LEXER_PLUS || context_p->stack_top_uint8 == LEXER_NEGATE)
            && !lexer_check_post_primary_exp (context_p))
        {
          do
          {
            if (context_p->stack_top_uint8 == LEXER_NEGATE)
            {
              is_negative_number = !is_negative_number;
            }
#if JERRY_BUILTIN_BIGINT
            else if (JERRY_LIKELY (context_p->token.extra_value == LEXER_NUMBER_BIGINT))
            {
              break;
            }
#endif /* JERRY_BUILTIN_BIGINT */
            parser_stack_pop_uint8 (context_p);
          }
          while (context_p->stack_top_uint8 == LEXER_PLUS
                 || context_p->stack_top_uint8 == LEXER_NEGATE);
        }

        if (lexer_construct_number_object (context_p, true, is_negative_number))
        {
          JERRY_ASSERT (context_p->lit_object.index <= CBC_PUSH_NUMBER_BYTE_RANGE_END);

          parser_emit_cbc_push_number (context_p, is_negative_number);
          break;
        }
      }

      cbc_opcode_t opcode = CBC_PUSH_LITERAL;

      if (context_p->token.keyword_type != LEXER_KEYW_EVAL)
      {
        if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
        {
          context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
          context_p->last_cbc.value = context_p->lit_object.index;
          context_p->last_cbc.literal_type = context_p->token.lit_location.type;
          context_p->last_cbc.literal_keyword_type = context_p->token.keyword_type;
          break;
        }

        if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
        {
          context_p->last_cbc_opcode = CBC_PUSH_THREE_LITERALS;
          context_p->last_cbc.third_literal_index = context_p->lit_object.index;
          context_p->last_cbc.literal_type = context_p->token.lit_location.type;
          context_p->last_cbc.literal_keyword_type = context_p->token.keyword_type;
          break;
        }

        if (context_p->last_cbc_opcode == CBC_PUSH_THIS)
        {
          context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
          opcode = CBC_PUSH_THIS_LITERAL;
        }
      }

      parser_emit_cbc_literal_from_token (context_p, (uint16_t) opcode);
      break;
    }
    case LEXER_KEYW_FUNCTION:
    {
      parser_parse_function_expression (context_p, PARSER_FUNCTION_CLOSURE | PARSER_IS_FUNC_EXPRESSION);
      break;
    }
    case LEXER_LEFT_BRACE:
    {
#if JERRY_ESNEXT
      if (context_p->next_scanner_info_p->source_p == context_p->source_p
          && context_p->next_scanner_info_p->type == SCANNER_TYPE_INITIALIZER)
      {
        if (parser_is_assignment_expr (context_p))
        {
          uint32_t flags = PARSER_PATTERN_NO_OPTS;

          if (context_p->next_scanner_info_p->u8_arg & SCANNER_LITERAL_OBJECT_HAS_REST)
          {
            flags |= PARSER_PATTERN_HAS_REST_ELEMENT;
          }

          parser_parse_object_initializer (context_p, flags);
          return parser_abort_parsing_after_assignment_expression (context_p);
        }

        scanner_release_next (context_p, sizeof (scanner_location_info_t));
      }
#endif /* JERRY_ESNEXT */

      parser_parse_object_literal (context_p);
      break;
    }
    case LEXER_LEFT_SQUARE:
    {
#if JERRY_ESNEXT
      if (context_p->next_scanner_info_p->source_p == context_p->source_p)
      {
        if (context_p->next_scanner_info_p->type == SCANNER_TYPE_INITIALIZER)
        {
          if (parser_is_assignment_expr (context_p))
          {
            parser_parse_array_initializer (context_p, PARSER_PATTERN_NO_OPTS);
            return parser_abort_parsing_after_assignment_expression (context_p);
          }

          scanner_release_next (context_p, sizeof (scanner_location_info_t));
        }
        else
        {
          JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_LITERAL_FLAGS
                        && context_p->next_scanner_info_p->u8_arg == SCANNER_LITERAL_NO_DESTRUCTURING);

          scanner_release_next (context_p, sizeof (scanner_info_t));
        }
      }
#endif /* JERRY_ESNEXT */

      parser_parse_array_literal (context_p);
      break;
    }
    case LEXER_DIVIDE:
    case LEXER_ASSIGN_DIVIDE:
    {
      lexer_construct_regexp_object (context_p, false);
      uint16_t literal_index = (uint16_t) (context_p->literal_count - 1);

      if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
      {
        context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
        context_p->last_cbc.value = literal_index;
      }
      else if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
      {
        context_p->last_cbc_opcode = CBC_PUSH_THREE_LITERALS;
        context_p->last_cbc.third_literal_index = literal_index;
      }
      else
      {
        parser_emit_cbc_literal (context_p, CBC_PUSH_LITERAL, literal_index);
      }

      context_p->last_cbc.literal_type = LEXER_REGEXP_LITERAL;
      context_p->last_cbc.literal_keyword_type = LEXER_EOS;
      break;
    }
    case LEXER_KEYW_THIS:
    {
#if JERRY_ESNEXT
      if (context_p->status_flags & PARSER_ALLOW_SUPER_CALL)
      {
        parser_emit_cbc_ext (context_p, CBC_EXT_RESOLVE_LEXICAL_THIS);
      }
      else
      {
#endif /* JERRY_ESNEXT */
        parser_emit_cbc (context_p, CBC_PUSH_THIS);
#if JERRY_ESNEXT
      }
#endif /* JERRY_ESNEXT */
      break;
    }
    case LEXER_LIT_TRUE:
    {
      parser_emit_cbc (context_p, CBC_PUSH_TRUE);
      break;
    }
    case LEXER_LIT_FALSE:
    {
      parser_emit_cbc (context_p, CBC_PUSH_FALSE);
      break;
    }
    case LEXER_LIT_NULL:
    {
      parser_emit_cbc (context_p, CBC_PUSH_NULL);
      break;
    }
#if JERRY_ESNEXT
    case LEXER_KEYW_CLASS:
    {
      parser_parse_class (context_p, false);
      return false;
    }
    case LEXER_KEYW_SUPER:
    {
      if (context_p->status_flags & PARSER_ALLOW_SUPER)
      {
        if (lexer_check_next_characters (context_p, LIT_CHAR_DOT, LIT_CHAR_LEFT_SQUARE))
        {
          parser_emit_cbc_ext (context_p, CBC_EXT_PUSH_SUPER);
          break;
        }

        if (lexer_check_next_character (context_p, LIT_CHAR_LEFT_PAREN)
            && (context_p->status_flags & PARSER_ALLOW_SUPER_CALL))
        {
          parser_emit_cbc_ext (context_p, CBC_EXT_PUSH_SUPER_CONSTRUCTOR);
          break;
        }
      }

      parser_raise_error (context_p, PARSER_ERR_UNEXPECTED_SUPER_KEYWORD);
    }
    case LEXER_LEFT_PAREN:
    {
      JERRY_ASSERT (context_p->next_scanner_info_p->source_p == context_p->source_p
                    && context_p->next_scanner_info_p->type == SCANNER_TYPE_FUNCTION);

      parser_check_assignment_expr (context_p);

      uint32_t arrow_status_flags = (PARSER_IS_FUNCTION
                                     | PARSER_IS_ARROW_FUNCTION
                                     | (context_p->status_flags & PARSER_INSIDE_CLASS_FIELD));
      parser_parse_function_expression (context_p, arrow_status_flags);
      return parser_abort_parsing_after_assignment_expression (context_p);
    }
    case LEXER_KEYW_YIELD:
    {
      JERRY_ASSERT ((context_p->status_flags & PARSER_IS_GENERATOR_FUNCTION)
                    && !(context_p->status_flags & PARSER_DISALLOW_AWAIT_YIELD));

      if (context_p->token.lit_location.has_escape)
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_KEYWORD);
      }

      parser_check_assignment_expr (context_p);
      lexer_next_token (context_p);

      cbc_ext_opcode_t opcode = ((context_p->status_flags & PARSER_IS_ASYNC_FUNCTION) ? CBC_EXT_ASYNC_YIELD
                                                                                      : CBC_EXT_YIELD);
      if (!lexer_check_yield_no_arg (context_p))
      {
        if (context_p->token.type == LEXER_MULTIPLY)
        {
          lexer_next_token (context_p);
          opcode = ((context_p->status_flags & PARSER_IS_ASYNC_FUNCTION) ? CBC_EXT_ASYNC_YIELD_ITERATOR
                                                                         : CBC_EXT_YIELD_ITERATOR);
        }

        parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);
      }
      else
      {
        parser_emit_cbc (context_p, CBC_PUSH_UNDEFINED);
      }

      parser_emit_cbc_ext (context_p, opcode);

      return (context_p->token.type != LEXER_RIGHT_PAREN
              && context_p->token.type != LEXER_COMMA);
    }
#endif /* JERRY_ESNEXT */
#if JERRY_MODULE_SYSTEM
    case LEXER_KEYW_IMPORT:
    {
      lexer_next_token (context_p);

      if (context_p->token.type != LEXER_LEFT_PAREN)
      {
        parser_raise_error (context_p, PARSER_ERR_LEFT_PAREN_EXPECTED);
      }

      lexer_next_token (context_p);
      parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);

      if (context_p->token.type != LEXER_RIGHT_PAREN)
      {
        parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
      }

      parser_emit_cbc_ext (context_p, CBC_EXT_MODULE_IMPORT);
      break;
    }
#endif /* JERRY_MODULE_SYSTEM */
    default:
    {
      bool is_left_hand_side = (*grouping_level_p == PARSE_EXPR_LEFT_HAND_SIDE);
      parser_raise_error (context_p, (is_left_hand_side ? PARSER_ERR_LEFT_HAND_SIDE_EXP_EXPECTED
                                                        : PARSER_ERR_PRIMARY_EXP_EXPECTED));
      break;
    }
  }
  lexer_next_token (context_p);
  return false;
} /* parser_parse_unary_expression */