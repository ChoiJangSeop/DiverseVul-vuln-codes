scanner_pop_literal_pool (parser_context_t *context_p, /**< context */
                          scanner_context_t *scanner_context_p) /**< scanner context */
{
  scanner_literal_pool_t *literal_pool_p = scanner_context_p->active_literal_pool_p;
  scanner_literal_pool_t *prev_literal_pool_p = literal_pool_p->prev_p;

#if JERRY_ESNEXT
  const uint32_t arrow_super_flags = (SCANNER_LITERAL_POOL_ARROW | SCANNER_LITERAL_POOL_HAS_SUPER_REFERENCE);
  if ((literal_pool_p->status_flags & arrow_super_flags) == arrow_super_flags)
  {
    prev_literal_pool_p->status_flags |= SCANNER_LITERAL_POOL_HAS_SUPER_REFERENCE;
  }
#endif /* JERRY_ESNEXT */

  if (JERRY_UNLIKELY (literal_pool_p->source_p == NULL))
  {
    JERRY_ASSERT (literal_pool_p->status_flags & SCANNER_LITERAL_POOL_FUNCTION);
    JERRY_ASSERT (literal_pool_p->literal_pool.data.first_p == NULL
                  && literal_pool_p->literal_pool.data.last_p == NULL);

    scanner_context_p->active_literal_pool_p = literal_pool_p->prev_p;
    scanner_free (literal_pool_p, sizeof (scanner_literal_pool_t));
    return;
  }

  uint16_t status_flags = literal_pool_p->status_flags;
  scanner_arguments_type_t arguments_type = SCANNER_ARGUMENTS_MAY_PRESENT;

  if (status_flags & SCANNER_LITERAL_POOL_NO_ARGUMENTS)
  {
    arguments_type = SCANNER_ARGUMENTS_NOT_PRESENT;
  }
  else if (status_flags & SCANNER_LITERAL_POOL_CAN_EVAL)
  {
    arguments_type = SCANNER_ARGUMENTS_MAY_PRESENT_IN_EVAL;
  }

#if JERRY_ESNEXT
  if (status_flags & SCANNER_LITERAL_POOL_ARGUMENTS_IN_ARGS)
  {
    arguments_type = SCANNER_ARGUMENTS_PRESENT;

    if (status_flags & (SCANNER_LITERAL_POOL_NO_ARGUMENTS | SCANNER_LITERAL_POOL_CAN_EVAL))
    {
      arguments_type = SCANNER_ARGUMENTS_PRESENT_NO_REG;
      status_flags &= (uint16_t) ~SCANNER_LITERAL_POOL_NO_ARGUMENTS;
    }
  }
#endif /* JERRY_ESNEXT */

  uint8_t can_eval_types = 0;
#if JERRY_ESNEXT
  if (prev_literal_pool_p == NULL && !(context_p->global_status_flags & ECMA_PARSE_DIRECT_EVAL))
  {
    can_eval_types |= SCANNER_LITERAL_IS_FUNC;
  }
#endif /* JERRY_ESNEXT */

  if ((status_flags & SCANNER_LITERAL_POOL_CAN_EVAL) && prev_literal_pool_p != NULL)
  {
    prev_literal_pool_p->status_flags |= SCANNER_LITERAL_POOL_CAN_EVAL;
  }

#if JERRY_DEBUGGER
  if (scanner_context_p->status_flags & SCANNER_CONTEXT_DEBUGGER_ENABLED)
  {
    /* When debugger is enabled, identifiers are not stored in registers. However,
     * this does not affect 'eval' detection, so 'arguments' object is not created. */
    status_flags |= SCANNER_LITERAL_POOL_CAN_EVAL;
  }
#endif /* JERRY_DEBUGGER */

  parser_list_iterator_t literal_iterator;
  lexer_lit_location_t *literal_p;
  int32_t no_declarations = literal_pool_p->no_declarations;

  parser_list_iterator_init (&literal_pool_p->literal_pool, &literal_iterator);

  uint8_t arguments_stream_type = SCANNER_STREAM_TYPE_ARGUMENTS;
  const uint8_t *prev_source_p = literal_pool_p->source_p - 1;
  lexer_lit_location_t *last_argument_p = NULL;
  size_t compressed_size = 1;

  while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
  {
    uint8_t type = literal_p->type;

    if (JERRY_UNLIKELY (no_declarations > PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK))
    {
      continue;
    }

    if (!(status_flags & SCANNER_LITERAL_POOL_NO_ARGUMENTS) && scanner_literal_is_arguments (literal_p))
    {
#if JERRY_ESNEXT
      JERRY_ASSERT (arguments_type != SCANNER_ARGUMENTS_NOT_PRESENT);
#else /* !JERRY_ESNEXT */
      JERRY_ASSERT (arguments_type == SCANNER_ARGUMENTS_MAY_PRESENT
                    || arguments_type == SCANNER_ARGUMENTS_MAY_PRESENT_IN_EVAL);
#endif /* JERRY_ESNEXT */

      status_flags |= SCANNER_LITERAL_POOL_NO_ARGUMENTS;

      if (type & SCANNER_LITERAL_IS_ARG)
      {
        JERRY_ASSERT (arguments_type != SCANNER_ARGUMENTS_PRESENT
                      && arguments_type != SCANNER_ARGUMENTS_PRESENT_NO_REG);
        arguments_type = SCANNER_ARGUMENTS_NOT_PRESENT;
        last_argument_p = literal_p;
      }
#if JERRY_ESNEXT
      else if (type & SCANNER_LITERAL_IS_LOCAL)
      {
        if (arguments_type == SCANNER_ARGUMENTS_MAY_PRESENT || arguments_type == SCANNER_ARGUMENTS_MAY_PRESENT_IN_EVAL)
        {
          arguments_type = SCANNER_ARGUMENTS_NOT_PRESENT;
        }
        else
        {
          if (arguments_type == SCANNER_ARGUMENTS_PRESENT_NO_REG)
          {
            type |= SCANNER_LITERAL_NO_REG;
          }
          else if (type & (SCANNER_LITERAL_NO_REG | SCANNER_LITERAL_EARLY_CREATE))
          {
            arguments_type = SCANNER_ARGUMENTS_PRESENT_NO_REG;
          }

          if ((type & SCANNER_LITERAL_IS_LOCAL_FUNC) == SCANNER_LITERAL_IS_LOCAL_FUNC)
          {
            type |= SCANNER_LITERAL_IS_ARG;
            literal_p->type = type;
            no_declarations--;
            arguments_stream_type = SCANNER_STREAM_TYPE_ARGUMENTS_FUNC;
          }
          else
          {
            arguments_stream_type |= SCANNER_STREAM_LOCAL_ARGUMENTS;
          }
        }
      }
#else /* !JERRY_ESNEXT */
      else if (type & SCANNER_LITERAL_IS_FUNC)
      {
        arguments_type = SCANNER_ARGUMENTS_NOT_PRESENT;
      }
#endif /* JERRY_ESNEXT */
      else
      {
#if JERRY_ESNEXT
        if ((type & SCANNER_LITERAL_IS_VAR)
            && (arguments_type == SCANNER_ARGUMENTS_PRESENT || arguments_type == SCANNER_ARGUMENTS_PRESENT_NO_REG))
        {
          if (arguments_type == SCANNER_ARGUMENTS_PRESENT_NO_REG)
          {
            type |= SCANNER_LITERAL_NO_REG;
          }
          else if (type & (SCANNER_LITERAL_NO_REG | SCANNER_LITERAL_EARLY_CREATE))
          {
            arguments_type = SCANNER_ARGUMENTS_PRESENT_NO_REG;
          }

          type |= SCANNER_LITERAL_IS_ARG;
          literal_p->type = type;
          no_declarations--;
        }
#endif /* JERRY_ESNEXT */

        if ((type & SCANNER_LITERAL_NO_REG) || arguments_type == SCANNER_ARGUMENTS_MAY_PRESENT_IN_EVAL)
        {
          arguments_type = SCANNER_ARGUMENTS_PRESENT_NO_REG;
        }
        else if (arguments_type == SCANNER_ARGUMENTS_MAY_PRESENT)
        {
          arguments_type = SCANNER_ARGUMENTS_PRESENT;
        }

#if JERRY_ESNEXT
        /* The SCANNER_LITERAL_IS_ARG may be set above. */
        if (!(type & SCANNER_LITERAL_IS_ARG))
        {
          literal_p->type = 0;
          continue;
        }
#else /* !JERRY_ESNEXT */
        literal_p->type = 0;
        continue;
#endif /* JERRY_ESNEXT */
      }
    }
    else if (type & SCANNER_LITERAL_IS_ARG)
    {
      last_argument_p = literal_p;
    }

#if JERRY_ESNEXT
    if ((status_flags & SCANNER_LITERAL_POOL_FUNCTION)
        && (type & SCANNER_LITERAL_IS_LOCAL_FUNC) == SCANNER_LITERAL_IS_FUNC)
    {
      if (prev_literal_pool_p == NULL && scanner_scope_find_lexical_declaration (context_p, literal_p))
      {
        literal_p->type = 0;
        continue;
      }

      if (!(type & SCANNER_LITERAL_IS_ARG))
      {
        type |= SCANNER_LITERAL_IS_VAR;
      }

      type &= (uint8_t) ~SCANNER_LITERAL_IS_FUNC;
      literal_p->type = type;
    }
#endif /* JERRY_ESNEXT */

    if ((type & SCANNER_LITERAL_IS_LOCAL)
        || ((type & (SCANNER_LITERAL_IS_VAR | SCANNER_LITERAL_IS_ARG))
            && (status_flags & SCANNER_LITERAL_POOL_FUNCTION)))
    {
      JERRY_ASSERT ((status_flags & SCANNER_LITERAL_POOL_FUNCTION)
                    || !(literal_p->type & SCANNER_LITERAL_IS_ARG));

      if (literal_p->length == 0)
      {
        compressed_size += 1;
        continue;
      }

      no_declarations++;

      if ((status_flags & SCANNER_LITERAL_POOL_CAN_EVAL) || (type & can_eval_types))
      {
        type |= SCANNER_LITERAL_NO_REG;
        literal_p->type = type;
      }

      if (type & SCANNER_LITERAL_IS_FUNC)
      {
        no_declarations++;

#if JERRY_ESNEXT
        if ((type & (SCANNER_LITERAL_IS_CONST | SCANNER_LITERAL_IS_ARG)) == SCANNER_LITERAL_IS_CONST)
        {
          JERRY_ASSERT (type & SCANNER_LITERAL_IS_LET);

          /* Catch parameters cannot be functions. */
          literal_p->type = (uint8_t) (type & ~SCANNER_LITERAL_IS_FUNC);
          no_declarations--;
        }
#else /* !JERRY_ESNEXT */
        if (type & SCANNER_LITERAL_IS_LOCAL)
        {
          /* Catch parameters cannot be functions. */
          literal_p->type = (uint8_t) (type & ~SCANNER_LITERAL_IS_FUNC);
          no_declarations--;
        }
#endif /* JERRY_ESNEXT */
      }

      intptr_t diff = (intptr_t) (literal_p->char_p - prev_source_p);

      if (diff >= 1 && diff <= (intptr_t) UINT8_MAX)
      {
        compressed_size += 2 + 1;
      }
      else if (diff >= -(intptr_t) UINT8_MAX && diff <= (intptr_t) UINT16_MAX)
      {
        compressed_size += 2 + 2;
      }
      else
      {
        compressed_size += 2 + 1 + sizeof (const uint8_t *);
      }

      prev_source_p = literal_p->char_p + literal_p->length;

      if ((status_flags & SCANNER_LITERAL_POOL_FUNCTION)
#if JERRY_ESNEXT
          || ((type & SCANNER_LITERAL_IS_FUNC) && (status_flags & SCANNER_LITERAL_POOL_IS_STRICT))
#endif /* JERRY_ESNEXT */
          || !(type & (SCANNER_LITERAL_IS_VAR | SCANNER_LITERAL_IS_FUNC)))
      {
        continue;
      }
    }

    if (prev_literal_pool_p != NULL && literal_p->length > 0)
    {
      /* Propagate literal to upper level. */
      lexer_lit_location_t *literal_location_p = scanner_add_custom_literal (context_p,
                                                                             prev_literal_pool_p,
                                                                             literal_p);
      uint8_t extended_type = literal_location_p->type;

#if JERRY_ESNEXT
      const uint16_t no_reg_flags = (SCANNER_LITERAL_POOL_FUNCTION | SCANNER_LITERAL_POOL_CLASS_FIELD);
#else /* !JERRY_ESNEXT */
      const uint16_t no_reg_flags = SCANNER_LITERAL_POOL_FUNCTION;
#endif /* JERRY_ESNEXT */

      if ((status_flags & no_reg_flags) || (type & SCANNER_LITERAL_NO_REG))
      {
        extended_type |= SCANNER_LITERAL_NO_REG;
      }

#if JERRY_ESNEXT
      extended_type |= SCANNER_LITERAL_IS_USED;

      if (status_flags & SCANNER_LITERAL_POOL_FUNCTION_STATEMENT)
      {
        extended_type |= SCANNER_LITERAL_EARLY_CREATE;
      }

      const uint8_t mask = (SCANNER_LITERAL_IS_ARG | SCANNER_LITERAL_IS_LOCAL);

      if ((type & SCANNER_LITERAL_IS_ARG)
          || (literal_location_p->type & mask) == SCANNER_LITERAL_IS_LET
          || (literal_location_p->type & mask) == SCANNER_LITERAL_IS_CONST)
      {
        /* Clears the SCANNER_LITERAL_IS_VAR and SCANNER_LITERAL_IS_FUNC flags
         * for speculative arrow parameters and local (non-var) functions. */
        type = 0;
      }
#endif /* JERRY_ESNEXT */

      type = (uint8_t) (type & (SCANNER_LITERAL_IS_VAR | SCANNER_LITERAL_IS_FUNC));
      JERRY_ASSERT (type == 0 || !(status_flags & SCANNER_LITERAL_POOL_FUNCTION));

      literal_location_p->type = (uint8_t) (extended_type | type);
    }
  }

  if ((status_flags & SCANNER_LITERAL_POOL_FUNCTION) || (compressed_size > 1))
  {
    if (arguments_type == SCANNER_ARGUMENTS_MAY_PRESENT)
    {
      arguments_type = SCANNER_ARGUMENTS_NOT_PRESENT;
    }
    else if (arguments_type == SCANNER_ARGUMENTS_MAY_PRESENT_IN_EVAL)
    {
      arguments_type = SCANNER_ARGUMENTS_PRESENT_NO_REG;
    }

    if (arguments_type != SCANNER_ARGUMENTS_NOT_PRESENT)
    {
      compressed_size++;
    }

    compressed_size += sizeof (scanner_info_t);

    scanner_info_t *info_p;

    if (prev_literal_pool_p != NULL || scanner_context_p->end_arguments_p == NULL)
    {
      info_p = scanner_insert_info (context_p, literal_pool_p->source_p, compressed_size);
    }
    else
    {
      scanner_info_t *start_info_p = scanner_context_p->end_arguments_p;
      info_p = scanner_insert_info_before (context_p, literal_pool_p->source_p, start_info_p, compressed_size);
    }

    if (no_declarations > PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK)
    {
      no_declarations = PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK;
    }

    uint8_t *data_p = (uint8_t *) (info_p + 1);
    bool mapped_arguments = false;

    if (status_flags & SCANNER_LITERAL_POOL_FUNCTION)
    {
      info_p->type = SCANNER_TYPE_FUNCTION;

      uint8_t u8_arg = 0;

      if (arguments_type != SCANNER_ARGUMENTS_NOT_PRESENT)
      {
        u8_arg |= SCANNER_FUNCTION_ARGUMENTS_NEEDED;

        if (no_declarations < PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK)
        {
          no_declarations++;
        }

#if JERRY_ESNEXT
        const uint16_t is_unmapped = SCANNER_LITERAL_POOL_IS_STRICT | SCANNER_LITERAL_POOL_HAS_COMPLEX_ARGUMENT;
#else /* !JERRY_ESNEXT */
        const uint16_t is_unmapped = SCANNER_LITERAL_POOL_IS_STRICT;
#endif /* JERRY_ESNEXT */

        if (!(status_flags & is_unmapped))
        {
          mapped_arguments = true;
        }

        if (arguments_type == SCANNER_ARGUMENTS_PRESENT_NO_REG)
        {
          arguments_stream_type |= SCANNER_STREAM_NO_REG;
        }

        if (last_argument_p == NULL)
        {
          *data_p++ = arguments_stream_type;
        }
      }
      else
      {
        last_argument_p = NULL;
      }

#if JERRY_ESNEXT
      if (status_flags & (SCANNER_LITERAL_POOL_HAS_COMPLEX_ARGUMENT | SCANNER_LITERAL_POOL_ARROW))
      {
        u8_arg |= SCANNER_FUNCTION_HAS_COMPLEX_ARGUMENT;
      }

      if (status_flags & SCANNER_LITERAL_POOL_ASYNC)
      {
        u8_arg |= SCANNER_FUNCTION_ASYNC;

        if (status_flags & SCANNER_LITERAL_POOL_FUNCTION_STATEMENT)
        {
          u8_arg |= SCANNER_FUNCTION_STATEMENT;
        }
      }

      if (status_flags & SCANNER_LITERAL_POOL_CAN_EVAL)
      {
        u8_arg |= SCANNER_FUNCTION_LEXICAL_ENV_NEEDED;
      }

      if (status_flags & SCANNER_LITERAL_POOL_IS_STRICT)
      {
        u8_arg |= SCANNER_FUNCTION_IS_STRICT;
      }
#endif /* JERRY_ESNEXT */

      info_p->u8_arg = u8_arg;
      info_p->u16_arg = (uint16_t) no_declarations;
    }
    else
    {
      info_p->type = SCANNER_TYPE_BLOCK;

      JERRY_ASSERT (prev_literal_pool_p != NULL);
    }

    parser_list_iterator_init (&literal_pool_p->literal_pool, &literal_iterator);
    prev_source_p = literal_pool_p->source_p - 1;
    no_declarations = literal_pool_p->no_declarations;

    while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
    {
      if (JERRY_UNLIKELY (no_declarations > PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK)
          || (!(literal_p->type & SCANNER_LITERAL_IS_LOCAL)
              && (!(literal_p->type & (SCANNER_LITERAL_IS_VAR | SCANNER_LITERAL_IS_ARG))
                  || !(status_flags & SCANNER_LITERAL_POOL_FUNCTION))))
      {
        continue;
      }

      if (literal_p->length == 0)
      {
        *data_p++ = SCANNER_STREAM_TYPE_HOLE;

        if (literal_p == last_argument_p)
        {
          *data_p++ = arguments_stream_type;
        }
        continue;
      }

      no_declarations++;

      uint8_t type = SCANNER_STREAM_TYPE_VAR;

      if (literal_p->type & SCANNER_LITERAL_IS_FUNC)
      {
        no_declarations++;
        type = SCANNER_STREAM_TYPE_FUNC;

        if (literal_p->type & SCANNER_LITERAL_IS_ARG)
        {
          type = SCANNER_STREAM_TYPE_ARG_FUNC;

#if JERRY_ESNEXT
          if (literal_p->type & SCANNER_LITERAL_IS_DESTRUCTURED_ARG)
          {
            type = SCANNER_STREAM_TYPE_DESTRUCTURED_ARG_FUNC;
          }
#endif /* JERRY_ESNEXT */
        }
      }
      else if (literal_p->type & SCANNER_LITERAL_IS_ARG)
      {
        type = SCANNER_STREAM_TYPE_ARG;

#if JERRY_ESNEXT
        if (literal_p->type & SCANNER_LITERAL_IS_DESTRUCTURED_ARG)
        {
          type = SCANNER_STREAM_TYPE_DESTRUCTURED_ARG;
        }

        if (literal_p->type & SCANNER_LITERAL_IS_VAR)
        {
          type = (uint8_t) (type + 1);

          JERRY_ASSERT (type == SCANNER_STREAM_TYPE_ARG_VAR
                        || type == SCANNER_STREAM_TYPE_DESTRUCTURED_ARG_VAR);
        }
#endif /* JERRY_ESNEXT */
      }
#if JERRY_ESNEXT
      else if (literal_p->type & SCANNER_LITERAL_IS_LET)
      {
        if (!(literal_p->type & SCANNER_LITERAL_IS_CONST))
        {
          type = SCANNER_STREAM_TYPE_LET;

          if ((status_flags & SCANNER_LITERAL_POOL_CAN_EVAL) && (literal_p->type & SCANNER_LITERAL_NO_REG))
          {
            literal_p->type |= SCANNER_LITERAL_EARLY_CREATE;
          }
        }
#if JERRY_MODULE_SYSTEM
        else if (prev_literal_pool_p == NULL)
        {
          type = SCANNER_STREAM_TYPE_IMPORT;
        }
#endif /* JERRY_MODULE_SYSTEM */
        else
        {
          type = SCANNER_STREAM_TYPE_LOCAL;
        }
      }
      else if (literal_p->type & SCANNER_LITERAL_IS_CONST)
      {
        type = SCANNER_STREAM_TYPE_CONST;

        if ((status_flags & SCANNER_LITERAL_POOL_CAN_EVAL) && (literal_p->type & SCANNER_LITERAL_NO_REG))
        {
          literal_p->type |= SCANNER_LITERAL_EARLY_CREATE;
        }
      }

      if (literal_p->type & SCANNER_LITERAL_EARLY_CREATE)
      {
        type |= SCANNER_STREAM_NO_REG | SCANNER_STREAM_EARLY_CREATE;
      }
#endif /* JERRY_ESNEXT */

      if (literal_p->has_escape)
      {
        type |= SCANNER_STREAM_HAS_ESCAPE;
      }

      if ((literal_p->type & SCANNER_LITERAL_NO_REG)
          || (mapped_arguments && (literal_p->type & SCANNER_LITERAL_IS_ARG)))
      {
        type |= SCANNER_STREAM_NO_REG;
      }

      data_p[0] = type;
      data_p[1] = (uint8_t) literal_p->length;
      data_p += 3;

      intptr_t diff = (intptr_t) (literal_p->char_p - prev_source_p);

      if (diff >= 1 && diff <= (intptr_t) UINT8_MAX)
      {
        data_p[-1] = (uint8_t) diff;
      }
      else if (diff >= -(intptr_t) UINT8_MAX && diff <= (intptr_t) UINT16_MAX)
      {
        if (diff < 0)
        {
          diff = -diff;
        }

        data_p[-3] |= SCANNER_STREAM_UINT16_DIFF;
        data_p[-1] = (uint8_t) diff;
        data_p[0] = (uint8_t) (diff >> 8);
        data_p += 1;
      }
      else
      {
        data_p[-1] = 0;
        memcpy (data_p, &literal_p->char_p, sizeof (uintptr_t));
        data_p += sizeof (uintptr_t);
      }

      if (literal_p == last_argument_p)
      {
        *data_p++ = arguments_stream_type;
      }

      prev_source_p = literal_p->char_p + literal_p->length;
    }

    data_p[0] = SCANNER_STREAM_TYPE_END;

    JERRY_ASSERT (((uint8_t *) info_p) + compressed_size == data_p + 1);
  }

  if (!(status_flags & SCANNER_LITERAL_POOL_FUNCTION)
      && (int32_t) prev_literal_pool_p->no_declarations < no_declarations)
  {
    prev_literal_pool_p->no_declarations = (uint16_t) no_declarations;
  }

  if ((status_flags & SCANNER_LITERAL_POOL_FUNCTION) && prev_literal_pool_p != NULL)
  {
    if (prev_literal_pool_p->status_flags & SCANNER_LITERAL_POOL_IS_STRICT)
    {
      context_p->status_flags |= PARSER_IS_STRICT;
    }
    else
    {
      context_p->status_flags &= (uint32_t) ~PARSER_IS_STRICT;
    }

#if JERRY_ESNEXT
    if (prev_literal_pool_p->status_flags & SCANNER_LITERAL_POOL_GENERATOR)
    {
      context_p->status_flags |= PARSER_IS_GENERATOR_FUNCTION;
    }
    else
    {
      context_p->status_flags &= (uint32_t) ~PARSER_IS_GENERATOR_FUNCTION;
    }

    if (prev_literal_pool_p->status_flags & SCANNER_LITERAL_POOL_ASYNC)
    {
      context_p->status_flags |= PARSER_IS_ASYNC_FUNCTION;
    }
    else
    {
      context_p->status_flags &= (uint32_t) ~PARSER_IS_ASYNC_FUNCTION;
    }
#endif /* JERRY_ESNEXT */
  }

  scanner_context_p->active_literal_pool_p = literal_pool_p->prev_p;

  parser_list_free (&literal_pool_p->literal_pool);
  scanner_free (literal_pool_p, sizeof (scanner_literal_pool_t));
} /* scanner_pop_literal_pool */