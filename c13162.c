scanner_create_variables (parser_context_t *context_p, /**< context */
                          uint32_t option_flags) /**< combination of scanner_create_variables_flags_t bits */
{
  scanner_info_t *info_p = context_p->next_scanner_info_p;
  const uint8_t *next_data_p = (const uint8_t *) (info_p + 1);
  uint8_t info_type = info_p->type;
  uint8_t info_u8_arg = info_p->u8_arg;
  lexer_lit_location_t literal;
  parser_scope_stack_t *scope_stack_p;
  parser_scope_stack_t *scope_stack_end_p;

  JERRY_ASSERT (info_type == SCANNER_TYPE_FUNCTION || info_type == SCANNER_TYPE_BLOCK);
  JERRY_ASSERT (!(option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_ARGS)
                || !(option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_BODY));
  JERRY_ASSERT (info_type == SCANNER_TYPE_FUNCTION
                || !(option_flags & (SCANNER_CREATE_VARS_IS_FUNCTION_ARGS | SCANNER_CREATE_VARS_IS_FUNCTION_BODY)));

  if (info_type == SCANNER_TYPE_FUNCTION && !(option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_BODY))
  {
    JERRY_ASSERT (context_p->scope_stack_p == NULL);

    size_t stack_size = info_p->u16_arg * sizeof (parser_scope_stack_t);
    context_p->scope_stack_size = info_p->u16_arg;

    scope_stack_p = NULL;

    if (stack_size > 0)
    {
      scope_stack_p = (parser_scope_stack_t *) parser_malloc (context_p, stack_size);
    }

    context_p->scope_stack_p = scope_stack_p;
    scope_stack_end_p = scope_stack_p + context_p->scope_stack_size;
  }
  else
  {
    JERRY_ASSERT (context_p->scope_stack_p != NULL || context_p->scope_stack_size == 0);

    scope_stack_p = context_p->scope_stack_p;
    scope_stack_end_p = scope_stack_p + context_p->scope_stack_size;
    scope_stack_p += context_p->scope_stack_top;
  }

  uint32_t scope_stack_reg_top = context_p->scope_stack_reg_top;

  literal.char_p = info_p->source_p - 1;

  while (next_data_p[0] != SCANNER_STREAM_TYPE_END)
  {
    uint32_t type = next_data_p[0] & SCANNER_STREAM_TYPE_MASK;
    const uint8_t *data_p = next_data_p;

    JERRY_ASSERT ((option_flags & (SCANNER_CREATE_VARS_IS_FUNCTION_BODY | SCANNER_CREATE_VARS_IS_FUNCTION_ARGS))
                  || (type != SCANNER_STREAM_TYPE_HOLE
                      && !SCANNER_STREAM_TYPE_IS_ARG (type)
                      && !SCANNER_STREAM_TYPE_IS_ARG_FUNC (type)));

#if JERRY_MODULE_SYSTEM
    JERRY_ASSERT (type != SCANNER_STREAM_TYPE_IMPORT || (data_p[0] & SCANNER_STREAM_NO_REG));
#endif /* JERRY_MODULE_SYSTEM */

    if (JERRY_UNLIKELY (type == SCANNER_STREAM_TYPE_HOLE))
    {
      JERRY_ASSERT (info_type == SCANNER_TYPE_FUNCTION);
      next_data_p++;

      if (option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_BODY)
      {
        continue;
      }

      uint8_t mask = SCANNER_FUNCTION_ARGUMENTS_NEEDED | SCANNER_FUNCTION_HAS_COMPLEX_ARGUMENT;

      if (!(context_p->status_flags & PARSER_IS_STRICT)
          && (info_u8_arg & mask) == SCANNER_FUNCTION_ARGUMENTS_NEEDED)
      {
        scanner_create_unused_literal (context_p, LEXER_FLAG_FUNCTION_ARGUMENT);
      }

      if (scope_stack_reg_top < PARSER_MAXIMUM_NUMBER_OF_REGISTERS)
      {
        scope_stack_reg_top++;
      }
      continue;
    }

    if (JERRY_UNLIKELY (SCANNER_STREAM_TYPE_IS_ARGUMENTS (type)))
    {
      JERRY_ASSERT (info_type == SCANNER_TYPE_FUNCTION);
      next_data_p++;

      if (option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_BODY)
      {
        continue;
      }

      context_p->status_flags |= PARSER_ARGUMENTS_NEEDED;

      if (JERRY_UNLIKELY (scope_stack_p >= scope_stack_end_p))
      {
        JERRY_ASSERT (context_p->scope_stack_size == PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK);
        parser_raise_error (context_p, PARSER_ERR_SCOPE_STACK_LIMIT_REACHED);
      }

      lexer_construct_literal_object (context_p, &lexer_arguments_literal, LEXER_NEW_IDENT_LITERAL);
      scope_stack_p->map_from = context_p->lit_object.index;

      uint16_t map_to;

      if (!(data_p[0] & SCANNER_STREAM_NO_REG)
          && scope_stack_reg_top < PARSER_MAXIMUM_NUMBER_OF_REGISTERS)
      {
        map_to = (uint16_t) (PARSER_REGISTER_START + scope_stack_reg_top);

#if JERRY_ESNEXT
        scope_stack_p->map_to = (uint16_t) (scope_stack_reg_top + 1);
#endif /* JERRY_ESNEXT */

        scope_stack_reg_top++;
      }
      else
      {
        context_p->lit_object.literal_p->status_flags |= LEXER_FLAG_USED;
        map_to = context_p->lit_object.index;

        context_p->status_flags |= PARSER_LEXICAL_ENV_NEEDED;

#if JERRY_ESNEXT
        if (data_p[0] & SCANNER_STREAM_LOCAL_ARGUMENTS)
        {
          context_p->status_flags |= PARSER_LEXICAL_BLOCK_NEEDED;
        }

        scope_stack_p->map_to = 0;
#endif /* JERRY_ESNEXT */
      }

#if !JERRY_ESNEXT
      scope_stack_p->map_to = map_to;
#endif /* !JERRY_ESNEXT */
      scope_stack_p++;

#if JERRY_PARSER_DUMP_BYTE_CODE
      context_p->scope_stack_top = (uint16_t) (scope_stack_p - context_p->scope_stack_p);
#endif /* JERRY_PARSER_DUMP_BYTE_CODE */

      parser_emit_cbc_ext_literal (context_p, CBC_EXT_CREATE_ARGUMENTS, map_to);

#if JERRY_ESNEXT
      if (type == SCANNER_STREAM_TYPE_ARGUMENTS_FUNC)
      {
        if (JERRY_UNLIKELY (scope_stack_p >= scope_stack_end_p))
        {
          JERRY_ASSERT (context_p->scope_stack_size == PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK);
          parser_raise_error (context_p, PARSER_ERR_SCOPE_STACK_LIMIT_REACHED);
        }

        scope_stack_p->map_from = PARSER_SCOPE_STACK_FUNC;
        scope_stack_p->map_to = context_p->literal_count;
        scope_stack_p++;

        scanner_create_unused_literal (context_p, 0);
      }
#endif /* JERRY_ESNEXT */

      if (option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_ARGS)
      {
        break;
      }
      continue;
    }

    JERRY_ASSERT (context_p->scope_stack_size != 0);

    if (!(data_p[0] & SCANNER_STREAM_UINT16_DIFF))
    {
      if (data_p[2] != 0)
      {
        literal.char_p += data_p[2];
        next_data_p += 2 + 1;
      }
      else
      {
        memcpy (&literal.char_p, data_p + 2 + 1, sizeof (uintptr_t));
        next_data_p += 2 + 1 + sizeof (uintptr_t);
      }
    }
    else
    {
      int32_t diff = ((int32_t) data_p[2]) | ((int32_t) data_p[3]) << 8;

      if (diff <= (intptr_t) UINT8_MAX)
      {
        diff = -diff;
      }

      literal.char_p += diff;
      next_data_p += 2 + 2;
    }

    if (SCANNER_STREAM_TYPE_IS_ARG (type))
    {
      if (option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_BODY)
      {
#if JERRY_ESNEXT
        if ((context_p->status_flags & PARSER_LEXICAL_BLOCK_NEEDED)
            && (type == SCANNER_STREAM_TYPE_ARG_VAR || type == SCANNER_STREAM_TYPE_DESTRUCTURED_ARG_VAR))
        {
          literal.length = data_p[1];
          literal.type = LEXER_IDENT_LITERAL;
          literal.has_escape = (data_p[0] & SCANNER_STREAM_HAS_ESCAPE) ? 1 : 0;

          /* Literal must be exists. */
          lexer_construct_literal_object (context_p, &literal, LEXER_IDENT_LITERAL);

          if (context_p->lit_object.index < PARSER_REGISTER_START)
          {
            parser_emit_cbc_ext_literal_from_token (context_p, CBC_EXT_COPY_FROM_ARG);
          }
        }
#endif /* JERRY_ESNEXT */

        literal.char_p += data_p[1];
        continue;
      }
    }
    else if ((option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_ARGS)
             && !SCANNER_STREAM_TYPE_IS_ARG_FUNC (type))
    {
      /* Function arguments must come first. */
      break;
    }

    literal.length = data_p[1];
    literal.type = LEXER_IDENT_LITERAL;
    literal.has_escape = (data_p[0] & SCANNER_STREAM_HAS_ESCAPE) ? 1 : 0;

    lexer_construct_literal_object (context_p, &literal, LEXER_NEW_IDENT_LITERAL);
    literal.char_p += data_p[1];

    if (SCANNER_STREAM_TYPE_IS_ARG_FUNC (type) && (option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_BODY))
    {
      JERRY_ASSERT (scope_stack_p >= context_p->scope_stack_p + 2);
      JERRY_ASSERT (context_p->status_flags & PARSER_IS_FUNCTION);
#if JERRY_ESNEXT
      JERRY_ASSERT (!(context_p->status_flags & PARSER_FUNCTION_IS_PARSING_ARGS));
#endif /* JERRY_ESNEXT */

      parser_scope_stack_t *function_map_p = scope_stack_p - 2;
      uint16_t literal_index = context_p->lit_object.index;

      while (literal_index != function_map_p->map_from)
      {
        function_map_p--;

        JERRY_ASSERT (function_map_p >= context_p->scope_stack_p);
      }

      JERRY_ASSERT (function_map_p[1].map_from == PARSER_SCOPE_STACK_FUNC);

      cbc_opcode_t opcode = CBC_SET_VAR_FUNC;

#if JERRY_ESNEXT
      if (JERRY_UNLIKELY (context_p->status_flags & PARSER_LEXICAL_BLOCK_NEEDED)
          && (function_map_p[0].map_to & PARSER_SCOPE_STACK_REGISTER_MASK) == 0)
      {
        opcode = CBC_INIT_ARG_OR_FUNC;
      }
#endif /* JERRY_ESNEXT */

      parser_emit_cbc_literal_value (context_p,
                                     (uint16_t) opcode,
                                     function_map_p[1].map_to,
                                     scanner_decode_map_to (function_map_p));
      continue;
    }

    if (JERRY_UNLIKELY (scope_stack_p >= scope_stack_end_p))
    {
      JERRY_ASSERT (context_p->scope_stack_size == PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK);
      parser_raise_error (context_p, PARSER_ERR_SCOPE_STACK_LIMIT_REACHED);
    }

    scope_stack_p->map_from = context_p->lit_object.index;

#if JERRY_ESNEXT
    if (info_type == SCANNER_TYPE_FUNCTION)
    {
      if (type != SCANNER_STREAM_TYPE_LET
#if JERRY_MODULE_SYSTEM
          && type != SCANNER_STREAM_TYPE_IMPORT
#endif /* JERRY_MODULE_SYSTEM */
          && type != SCANNER_STREAM_TYPE_CONST)
      {
        context_p->lit_object.literal_p->status_flags |= LEXER_FLAG_GLOBAL;
      }
    }
#endif /* JERRY_ESNEXT */

    uint16_t map_to;
    uint16_t func_init_opcode = CBC_INIT_ARG_OR_FUNC;

    if (!(data_p[0] & SCANNER_STREAM_NO_REG)
        && scope_stack_reg_top < PARSER_MAXIMUM_NUMBER_OF_REGISTERS)
    {
      map_to = (uint16_t) (PARSER_REGISTER_START + scope_stack_reg_top);

#if JERRY_ESNEXT
      scope_stack_p->map_to = (uint16_t) (scope_stack_reg_top + 1);
#else /* !JERRY_ESNEXT */
      scope_stack_p->map_to = map_to;
#endif /* JERRY_ESNEXT */

      scope_stack_reg_top++;
#if JERRY_ESNEXT
      switch (type)
      {
        case SCANNER_STREAM_TYPE_CONST:
        {
          scope_stack_p->map_to |= PARSER_SCOPE_STACK_IS_CONST_REG;
          /* FALLTHRU */
        }
        case SCANNER_STREAM_TYPE_LET:
        case SCANNER_STREAM_TYPE_ARG:
        case SCANNER_STREAM_TYPE_ARG_VAR:
        case SCANNER_STREAM_TYPE_DESTRUCTURED_ARG:
        case SCANNER_STREAM_TYPE_DESTRUCTURED_ARG_VAR:
        case SCANNER_STREAM_TYPE_ARG_FUNC:
        case SCANNER_STREAM_TYPE_DESTRUCTURED_ARG_FUNC:
        {
          scope_stack_p->map_to |= PARSER_SCOPE_STACK_NO_FUNCTION_COPY;
          break;
        }
      }

      func_init_opcode = CBC_SET_VAR_FUNC;
#endif /* JERRY_ESNEXT */
    }
    else
    {
      context_p->lit_object.literal_p->status_flags |= LEXER_FLAG_USED;
      map_to = context_p->lit_object.index;

#if JERRY_ESNEXT
      uint16_t scope_stack_map_to = 0;
#else /* !JERRY_ESNEXT */
      scope_stack_p->map_to = map_to;
#endif /* JERRY_ESNEXT */

      if (info_type == SCANNER_TYPE_FUNCTION)
      {
        context_p->status_flags |= PARSER_LEXICAL_ENV_NEEDED;
      }

      switch (type)
      {
#if JERRY_ESNEXT
        case SCANNER_STREAM_TYPE_LET:
        case SCANNER_STREAM_TYPE_CONST:
        case SCANNER_STREAM_TYPE_DESTRUCTURED_ARG:
        case SCANNER_STREAM_TYPE_DESTRUCTURED_ARG_VAR:
        case SCANNER_STREAM_TYPE_DESTRUCTURED_ARG_FUNC:
        {
          scope_stack_map_to |= PARSER_SCOPE_STACK_NO_FUNCTION_COPY;

          if (!(data_p[0] & SCANNER_STREAM_EARLY_CREATE))
          {
            break;
          }
          scope_stack_map_to |= PARSER_SCOPE_STACK_IS_LOCAL_CREATED;
          /* FALLTHRU */
        }
        case SCANNER_STREAM_TYPE_LOCAL:
#endif /* JERRY_ESNEXT */
        case SCANNER_STREAM_TYPE_VAR:
        {
#if JERRY_PARSER_DUMP_BYTE_CODE
          context_p->scope_stack_top = (uint16_t) (scope_stack_p - context_p->scope_stack_p);
#endif /* JERRY_PARSER_DUMP_BYTE_CODE */

#if JERRY_ESNEXT
          uint16_t opcode;

          switch (type)
          {
            case SCANNER_STREAM_TYPE_LET:
            {
              opcode = CBC_CREATE_LET;
              break;
            }
            case SCANNER_STREAM_TYPE_CONST:
            {
              opcode = CBC_CREATE_CONST;
              break;
            }
            case SCANNER_STREAM_TYPE_VAR:
            {
              opcode = CBC_CREATE_VAR;

              if (option_flags & SCANNER_CREATE_VARS_IS_SCRIPT)
              {
                opcode = CBC_CREATE_VAR_EVAL;

                if ((context_p->global_status_flags & ECMA_PARSE_FUNCTION_CONTEXT)
                    && !(context_p->status_flags & PARSER_IS_STRICT))
                {
                  opcode = PARSER_TO_EXT_OPCODE (CBC_EXT_CREATE_VAR_EVAL);
                }
              }
              break;
            }
            default:
            {
              JERRY_ASSERT (type == SCANNER_STREAM_TYPE_LOCAL
                            || type == SCANNER_STREAM_TYPE_DESTRUCTURED_ARG
                            || type == SCANNER_STREAM_TYPE_DESTRUCTURED_ARG_VAR
                            || type == SCANNER_STREAM_TYPE_DESTRUCTURED_ARG_FUNC);

              opcode = CBC_CREATE_LOCAL;
              break;
            }
          }
#else /* !JERRY_ESNEXT */
          uint16_t opcode = ((option_flags & SCANNER_CREATE_VARS_IS_SCRIPT) ? CBC_CREATE_VAR_EVAL
                                                                            : CBC_CREATE_VAR);
#endif /* JERRY_ESNEXT */

          parser_emit_cbc_literal (context_p, opcode, map_to);
          break;
        }
        case SCANNER_STREAM_TYPE_ARG:
#if JERRY_ESNEXT
        case SCANNER_STREAM_TYPE_ARG_VAR:
#endif /* JERRY_ESNEXT */
        case SCANNER_STREAM_TYPE_ARG_FUNC:
        {
#if JERRY_PARSER_DUMP_BYTE_CODE
          context_p->scope_stack_top = (uint16_t) (scope_stack_p - context_p->scope_stack_p);
#endif /* JERRY_PARSER_DUMP_BYTE_CODE */

#if JERRY_ESNEXT
          scope_stack_map_to |= PARSER_SCOPE_STACK_NO_FUNCTION_COPY;

          /* Argument initializers of functions with simple arguments (e.g. function f(a,b,a) {}) are
           * generated here. The other initializers are handled by parser_parse_function_arguments(). */
          if (!(info_u8_arg & SCANNER_FUNCTION_HAS_COMPLEX_ARGUMENT))
          {
#endif /* JERRY_ESNEXT */
            parser_emit_cbc_literal_value (context_p,
                                           CBC_INIT_ARG_OR_FUNC,
                                           (uint16_t) (PARSER_REGISTER_START + scope_stack_reg_top),
                                           map_to);
#if JERRY_ESNEXT
          }
          else if (data_p[0] & SCANNER_STREAM_EARLY_CREATE)
          {
            parser_emit_cbc_literal (context_p, CBC_CREATE_LOCAL, map_to);
            scope_stack_map_to |= PARSER_SCOPE_STACK_IS_LOCAL_CREATED;
          }
#endif /* JERRY_ESNEXT */

          if (scope_stack_reg_top < PARSER_MAXIMUM_NUMBER_OF_REGISTERS)
          {
            scope_stack_reg_top++;
          }
          break;
        }
      }

#if JERRY_ESNEXT
      scope_stack_p->map_to = scope_stack_map_to;
#endif /* JERRY_ESNEXT */
    }

    scope_stack_p++;

    if (!SCANNER_STREAM_TYPE_IS_FUNCTION (type))
    {
      continue;
    }

    if (JERRY_UNLIKELY (scope_stack_p >= scope_stack_end_p))
    {
      JERRY_ASSERT (context_p->scope_stack_size == PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK);
      parser_raise_error (context_p, PARSER_ERR_SCOPE_STACK_LIMIT_REACHED);
    }

#if JERRY_PARSER_DUMP_BYTE_CODE
    context_p->scope_stack_top = (uint16_t) (scope_stack_p - context_p->scope_stack_p);
#endif /* JERRY_PARSER_DUMP_BYTE_CODE */

    if (!SCANNER_STREAM_TYPE_IS_ARG_FUNC (type))
    {
      if (func_init_opcode == CBC_INIT_ARG_OR_FUNC && (option_flags & SCANNER_CREATE_VARS_IS_SCRIPT))
      {
#if JERRY_ESNEXT
        literal.char_p -= data_p[1];

        if (!scanner_scope_find_lexical_declaration (context_p, &literal))
        {
          func_init_opcode = CBC_CREATE_VAR_FUNC_EVAL;

          if (context_p->global_status_flags & ECMA_PARSE_FUNCTION_CONTEXT)
          {
            func_init_opcode = PARSER_TO_EXT_OPCODE (CBC_EXT_CREATE_VAR_FUNC_EVAL);
          }
        }
        literal.char_p += data_p[1];
#else /* !JERRY_ESNEXT */
        func_init_opcode = CBC_CREATE_VAR_FUNC_EVAL;
#endif /* JERRY_ESNEXT */
      }

      parser_emit_cbc_literal_value (context_p, func_init_opcode, context_p->literal_count, map_to);
    }

    scope_stack_p->map_from = PARSER_SCOPE_STACK_FUNC;
    scope_stack_p->map_to = context_p->literal_count;
    scope_stack_p++;

    scanner_create_unused_literal (context_p, 0);
  }

  context_p->scope_stack_top = (uint16_t) (scope_stack_p - context_p->scope_stack_p);
  context_p->scope_stack_reg_top = (uint16_t) scope_stack_reg_top;

#if JERRY_ESNEXT
  if (info_type == SCANNER_TYPE_FUNCTION)
  {
    context_p->scope_stack_global_end = context_p->scope_stack_top;
  }
#endif /* JERRY_ESNEXT */

  if (context_p->register_count < scope_stack_reg_top)
  {
    context_p->register_count = (uint16_t) scope_stack_reg_top;
  }

  if (!(option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_ARGS))
  {
    scanner_release_next (context_p, (size_t) (next_data_p + 1 - ((const uint8_t *) info_p)));
  }
  parser_flush_cbc (context_p);
} /* scanner_create_variables */