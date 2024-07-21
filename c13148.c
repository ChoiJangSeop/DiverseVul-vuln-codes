parser_post_processing (parser_context_t *context_p) /**< context */
{
  uint16_t literal_one_byte_limit;
  uint16_t ident_end;
  uint16_t const_literal_end;
  parser_mem_page_t *page_p;
  parser_mem_page_t *last_page_p;
  size_t last_position;
  size_t offset;
  size_t length;
  size_t literal_length;
  size_t total_size;
  uint8_t real_offset;
  uint8_t *byte_code_p;
  bool needs_uint16_arguments;
  cbc_opcode_t last_opcode = CBC_EXT_OPCODE;
  ecma_compiled_code_t *compiled_code_p;
  ecma_value_t *literal_pool_p;
  uint8_t *dst_p;

#if JERRY_ESNEXT
  if ((context_p->status_flags & (PARSER_IS_FUNCTION | PARSER_LEXICAL_BLOCK_NEEDED))
      == (PARSER_IS_FUNCTION | PARSER_LEXICAL_BLOCK_NEEDED))
  {
    PARSER_MINUS_EQUAL_U16 (context_p->stack_depth, PARSER_BLOCK_CONTEXT_STACK_ALLOCATION);
#ifndef JERRY_NDEBUG
    PARSER_MINUS_EQUAL_U16 (context_p->context_stack_depth, PARSER_BLOCK_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

    context_p->status_flags &= (uint32_t) ~PARSER_LEXICAL_BLOCK_NEEDED;

    parser_emit_cbc (context_p, CBC_CONTEXT_END);

    parser_branch_t branch;
    parser_stack_pop (context_p, &branch, sizeof (parser_branch_t));
    parser_set_branch_to_current_position (context_p, &branch);

    JERRY_ASSERT (!(context_p->status_flags & PARSER_NO_END_LABEL));
  }

  if (PARSER_IS_NORMAL_ASYNC_FUNCTION (context_p->status_flags))
  {
    PARSER_MINUS_EQUAL_U16 (context_p->stack_depth, PARSER_TRY_CONTEXT_STACK_ALLOCATION);
#ifndef JERRY_NDEBUG
    PARSER_MINUS_EQUAL_U16 (context_p->context_stack_depth, PARSER_TRY_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

    if (context_p->stack_limit < PARSER_FINALLY_CONTEXT_STACK_ALLOCATION)
    {
      context_p->stack_limit = PARSER_FINALLY_CONTEXT_STACK_ALLOCATION;
    }

    parser_branch_t branch;

    parser_stack_pop (context_p, &branch, sizeof (parser_branch_t));
    parser_set_branch_to_current_position (context_p, &branch);

    JERRY_ASSERT (!(context_p->status_flags & PARSER_NO_END_LABEL));
  }
#endif /* JERRY_ESNEXT */

#if JERRY_LINE_INFO
  JERRY_ASSERT (context_p->line_info_p != NULL);
#endif /* JERRY_LINE_INFO */

  JERRY_ASSERT (context_p->stack_depth == 0);
#ifndef JERRY_NDEBUG
  JERRY_ASSERT (context_p->context_stack_depth == 0);
#endif /* !JERRY_NDEBUG */

  if ((size_t) context_p->stack_limit + (size_t) context_p->register_count > PARSER_MAXIMUM_STACK_LIMIT)
  {
    parser_raise_error (context_p, PARSER_ERR_STACK_LIMIT_REACHED);
  }

  if (JERRY_UNLIKELY (context_p->script_p->refs_and_type >= CBC_SCRIPT_REF_MAX))
  {
    /* This is probably never happens in practice. */
    jerry_fatal (ERR_REF_COUNT_LIMIT);
  }

  context_p->script_p->refs_and_type += CBC_SCRIPT_REF_ONE;

  JERRY_ASSERT (context_p->literal_count <= PARSER_MAXIMUM_NUMBER_OF_LITERALS);

#if JERRY_DEBUGGER
  if ((JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
      && !(context_p->status_flags & PARSER_DEBUGGER_BREAKPOINT_APPENDED))
  {
    /* Always provide at least one breakpoint. */
    parser_emit_cbc (context_p, CBC_BREAKPOINT_DISABLED);
    parser_flush_cbc (context_p);

    parser_append_breakpoint_info (context_p, JERRY_DEBUGGER_BREAKPOINT_LIST, context_p->token.line);

    context_p->last_breakpoint_line = context_p->token.line;
  }

  if ((JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
      && context_p->breakpoint_info_count > 0)
  {
    parser_send_breakpoints (context_p, JERRY_DEBUGGER_BREAKPOINT_LIST);
    JERRY_ASSERT (context_p->breakpoint_info_count == 0);
  }
#endif /* JERRY_DEBUGGER */

  parser_compute_indicies (context_p, &ident_end, &const_literal_end);

  if (context_p->literal_count <= CBC_MAXIMUM_SMALL_VALUE)
  {
    literal_one_byte_limit = CBC_MAXIMUM_BYTE_VALUE - 1;
  }
  else
  {
    literal_one_byte_limit = CBC_LOWER_SEVEN_BIT_MASK;
  }

  last_page_p = context_p->byte_code.last_p;
  last_position = context_p->byte_code.last_position;

  if (last_position >= PARSER_CBC_STREAM_PAGE_SIZE)
  {
    last_page_p = NULL;
    last_position = 0;
  }

  page_p = context_p->byte_code.first_p;
  offset = 0;
  length = 0;

  while (page_p != last_page_p || offset < last_position)
  {
    uint8_t *opcode_p;
    uint8_t flags;
    size_t branch_offset_length;

    opcode_p = page_p->bytes + offset;
    last_opcode = (cbc_opcode_t) (*opcode_p);
    PARSER_NEXT_BYTE (page_p, offset);
    branch_offset_length = CBC_BRANCH_OFFSET_LENGTH (last_opcode);
    flags = cbc_flags[last_opcode];
    length++;

    switch (last_opcode)
    {
      case CBC_EXT_OPCODE:
      {
        cbc_ext_opcode_t ext_opcode;

        ext_opcode = (cbc_ext_opcode_t) page_p->bytes[offset];
        branch_offset_length = CBC_BRANCH_OFFSET_LENGTH (ext_opcode);
        flags = cbc_ext_flags[ext_opcode];
        PARSER_NEXT_BYTE (page_p, offset);
        length++;
        break;
      }
      case CBC_POST_DECR:
      {
        *opcode_p = CBC_PRE_DECR;
        break;
      }
      case CBC_POST_INCR:
      {
        *opcode_p = CBC_PRE_INCR;
        break;
      }
      case CBC_POST_DECR_IDENT:
      {
        *opcode_p = CBC_PRE_DECR_IDENT;
        break;
      }
      case CBC_POST_INCR_IDENT:
      {
        *opcode_p = CBC_PRE_INCR_IDENT;
        break;
      }
      default:
      {
        break;
      }
    }

    while (flags & (CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2))
    {
      uint8_t *first_byte = page_p->bytes + offset;
      uint32_t literal_index = *first_byte;

      PARSER_NEXT_BYTE (page_p, offset);
      length++;

      literal_index |= ((uint32_t) page_p->bytes[offset]) << 8;

      if (literal_index >= PARSER_REGISTER_START)
      {
        literal_index -= PARSER_REGISTER_START;
      }
      else
      {
        literal_index = (PARSER_GET_LITERAL (literal_index))->prop.index;
      }

      if (literal_index <= literal_one_byte_limit)
      {
        *first_byte = (uint8_t) literal_index;
      }
      else
      {
        if (context_p->literal_count <= CBC_MAXIMUM_SMALL_VALUE)
        {
          JERRY_ASSERT (literal_index <= CBC_MAXIMUM_SMALL_VALUE);
          *first_byte = CBC_MAXIMUM_BYTE_VALUE;
          page_p->bytes[offset] = (uint8_t) (literal_index - CBC_MAXIMUM_BYTE_VALUE);
          length++;
        }
        else
        {
          JERRY_ASSERT (literal_index <= CBC_MAXIMUM_FULL_VALUE);
          *first_byte = (uint8_t) ((literal_index >> 8) | CBC_HIGHEST_BIT_MASK);
          page_p->bytes[offset] = (uint8_t) (literal_index & 0xff);
          length++;
        }
      }
      PARSER_NEXT_BYTE (page_p, offset);

      if (flags & CBC_HAS_LITERAL_ARG2)
      {
        if (flags & CBC_HAS_LITERAL_ARG)
        {
          flags = CBC_HAS_LITERAL_ARG;
        }
        else
        {
          flags = CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2;
        }
      }
      else
      {
        break;
      }
    }

    if (flags & CBC_HAS_BYTE_ARG)
    {
      /* This argument will be copied without modification. */
      PARSER_NEXT_BYTE (page_p, offset);
      length++;
    }

    if (flags & CBC_HAS_BRANCH_ARG)
    {
      bool prefix_zero = true;

      /* The leading zeroes are dropped from the stream.
       * Although dropping these zeroes for backward
       * branches are unnecessary, we use the same
       * code path for simplicity. */
      JERRY_ASSERT (branch_offset_length > 0 && branch_offset_length <= 3);

      while (--branch_offset_length > 0)
      {
        uint8_t byte = page_p->bytes[offset];
        if (byte > 0 || !prefix_zero)
        {
          prefix_zero = false;
          length++;
        }
        else
        {
          JERRY_ASSERT (CBC_BRANCH_IS_FORWARD (flags));
        }
        PARSER_NEXT_BYTE (page_p, offset);
      }

      if (last_opcode == (cbc_opcode_t) (CBC_JUMP_FORWARD + PARSER_MAX_BRANCH_LENGTH - 1)
          && prefix_zero
          && page_p->bytes[offset] == PARSER_MAX_BRANCH_LENGTH + 1)
      {
        /* Uncoditional jumps which jump right after the instruction
         * are effectively NOPs. These jumps are removed from the
         * stream. The 1 byte long CBC_JUMP_FORWARD form marks these
         * instructions, since this form is constructed during post
         * processing and cannot be emitted directly. */
        *opcode_p = CBC_JUMP_FORWARD;
        length--;
      }
      else
      {
        /* Other last bytes are always copied. */
        length++;
      }

      PARSER_NEXT_BYTE (page_p, offset);
    }
  }

  if (!(context_p->status_flags & PARSER_NO_END_LABEL)
      || !(PARSER_OPCODE_IS_RETURN (last_opcode)))
  {
    context_p->status_flags &= (uint32_t) ~PARSER_NO_END_LABEL;

#if JERRY_ESNEXT
    if (PARSER_IS_NORMAL_ASYNC_FUNCTION (context_p->status_flags))
    {
      length++;
    }
#endif /* JERRY_ESNEXT */

    length++;
  }

  needs_uint16_arguments = false;
  total_size = sizeof (cbc_uint8_arguments_t);

  if (context_p->stack_limit > CBC_MAXIMUM_BYTE_VALUE
      || context_p->register_count > CBC_MAXIMUM_BYTE_VALUE
      || context_p->literal_count > CBC_MAXIMUM_BYTE_VALUE)
  {
    needs_uint16_arguments = true;
    total_size = sizeof (cbc_uint16_arguments_t);
  }

  literal_length = (size_t) (context_p->literal_count - context_p->register_count) * sizeof (ecma_value_t);

  total_size += literal_length + length;

  if (PARSER_NEEDS_MAPPED_ARGUMENTS (context_p->status_flags))
  {
    total_size += context_p->argument_count * sizeof (ecma_value_t);
  }

#if JERRY_ESNEXT
  /* function.name */
  if (!(context_p->status_flags & PARSER_CLASS_CONSTRUCTOR))
  {
    total_size += sizeof (ecma_value_t);
  }

  if (context_p->argument_length != UINT16_MAX)
  {
    total_size += sizeof (ecma_value_t);
  }

  if (context_p->tagged_template_literal_cp != JMEM_CP_NULL)
  {
    total_size += sizeof (ecma_value_t);
  }
#endif /* JERRY_ESNEXT */

#if JERRY_LINE_INFO
  total_size += sizeof (ecma_value_t);
#endif /* JERRY_LINE_INFO */

  total_size = JERRY_ALIGNUP (total_size, JMEM_ALIGNMENT);

  compiled_code_p = (ecma_compiled_code_t *) parser_malloc (context_p, total_size);

#if JERRY_SNAPSHOT_SAVE || JERRY_PARSER_DUMP_BYTE_CODE
  // Avoid getting junk bytes
  memset (compiled_code_p, 0, total_size);
#endif /* JERRY_SNAPSHOT_SAVE || JERRY_PARSER_DUMP_BYTE_CODE */

#if JERRY_MEM_STATS
  jmem_stats_allocate_byte_code_bytes (total_size);
#endif /* JERRY_MEM_STATS */

  byte_code_p = (uint8_t *) compiled_code_p;
  compiled_code_p->size = (uint16_t) (total_size >> JMEM_ALIGNMENT_LOG);
  compiled_code_p->refs = 1;
  compiled_code_p->status_flags = 0;

#if JERRY_ESNEXT
  if (context_p->status_flags & PARSER_FUNCTION_HAS_REST_PARAM)
  {
    JERRY_ASSERT (context_p->argument_count > 0);
    context_p->argument_count--;
  }
#endif /* JERRY_ESNEXT */

  if (needs_uint16_arguments)
  {
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) compiled_code_p;

    args_p->stack_limit = context_p->stack_limit;
    args_p->script_value = context_p->script_value;
    args_p->argument_end = context_p->argument_count;
    args_p->register_end = context_p->register_count;
    args_p->ident_end = ident_end;
    args_p->const_literal_end = const_literal_end;
    args_p->literal_end = context_p->literal_count;

    compiled_code_p->status_flags |= CBC_CODE_FLAGS_UINT16_ARGUMENTS;
    byte_code_p += sizeof (cbc_uint16_arguments_t);
  }
  else
  {
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) compiled_code_p;

    args_p->stack_limit = (uint8_t) context_p->stack_limit;
    args_p->argument_end = (uint8_t) context_p->argument_count;
    args_p->script_value = context_p->script_value;
    args_p->register_end = (uint8_t) context_p->register_count;
    args_p->ident_end = (uint8_t) ident_end;
    args_p->const_literal_end = (uint8_t) const_literal_end;
    args_p->literal_end = (uint8_t) context_p->literal_count;

    byte_code_p += sizeof (cbc_uint8_arguments_t);
  }

  uint16_t encoding_limit;
  uint16_t encoding_delta;

  if (context_p->literal_count > CBC_MAXIMUM_SMALL_VALUE)
  {
    compiled_code_p->status_flags |= CBC_CODE_FLAGS_FULL_LITERAL_ENCODING;
    encoding_limit = CBC_FULL_LITERAL_ENCODING_LIMIT;
    encoding_delta = CBC_FULL_LITERAL_ENCODING_DELTA;
  }
  else
  {
    encoding_limit = CBC_SMALL_LITERAL_ENCODING_LIMIT;
    encoding_delta = CBC_SMALL_LITERAL_ENCODING_DELTA;
  }

  if (context_p->status_flags & PARSER_IS_STRICT)
  {
    compiled_code_p->status_flags |= CBC_CODE_FLAGS_STRICT_MODE;
  }

  if ((context_p->status_flags & PARSER_ARGUMENTS_NEEDED)
      && PARSER_NEEDS_MAPPED_ARGUMENTS (context_p->status_flags))
  {
    compiled_code_p->status_flags |= CBC_CODE_FLAGS_MAPPED_ARGUMENTS_NEEDED;
  }

  if (!(context_p->status_flags & PARSER_LEXICAL_ENV_NEEDED))
  {
    compiled_code_p->status_flags |= CBC_CODE_FLAGS_LEXICAL_ENV_NOT_NEEDED;
  }

  uint16_t function_type = CBC_FUNCTION_TO_TYPE_BITS (CBC_FUNCTION_NORMAL);

  if (context_p->status_flags & (PARSER_IS_PROPERTY_GETTER | PARSER_IS_PROPERTY_SETTER))
  {
    function_type = CBC_FUNCTION_TO_TYPE_BITS (CBC_FUNCTION_ACCESSOR);
  }
  else if (!(context_p->status_flags & PARSER_IS_FUNCTION))
  {
    function_type = CBC_FUNCTION_TO_TYPE_BITS (CBC_FUNCTION_SCRIPT);
  }
#if JERRY_ESNEXT
  else if (context_p->status_flags & PARSER_IS_ARROW_FUNCTION)
  {
    if (context_p->status_flags & PARSER_IS_ASYNC_FUNCTION)
    {
      function_type = CBC_FUNCTION_TO_TYPE_BITS (CBC_FUNCTION_ASYNC_ARROW);
    }
    else
    {
      function_type = CBC_FUNCTION_TO_TYPE_BITS (CBC_FUNCTION_ARROW);
    }
  }
  else if (context_p->status_flags & PARSER_IS_GENERATOR_FUNCTION)
  {
    if (context_p->status_flags & PARSER_IS_ASYNC_FUNCTION)
    {
      function_type = CBC_FUNCTION_TO_TYPE_BITS (CBC_FUNCTION_ASYNC_GENERATOR);
    }
    else
    {
      function_type = CBC_FUNCTION_TO_TYPE_BITS (CBC_FUNCTION_GENERATOR);
    }
  }
  else if (context_p->status_flags & PARSER_IS_ASYNC_FUNCTION)
  {
    function_type = CBC_FUNCTION_TO_TYPE_BITS (CBC_FUNCTION_ASYNC);
  }
  else if (context_p->status_flags & PARSER_CLASS_CONSTRUCTOR)
  {
    function_type = CBC_FUNCTION_TO_TYPE_BITS (CBC_FUNCTION_CONSTRUCTOR);
  }
  else if (context_p->status_flags & PARSER_IS_METHOD)
  {
    function_type = CBC_FUNCTION_TO_TYPE_BITS (CBC_FUNCTION_METHOD);
  }

  if (context_p->status_flags & PARSER_LEXICAL_BLOCK_NEEDED)
  {
    JERRY_ASSERT (!(context_p->status_flags & PARSER_IS_FUNCTION));
    compiled_code_p->status_flags |= CBC_CODE_FLAGS_LEXICAL_BLOCK_NEEDED;
  }
#endif /* JERRY_ESNEXT */

  compiled_code_p->status_flags |= function_type;

#if JERRY_LINE_INFO
  compiled_code_p->status_flags |= CBC_CODE_FLAGS_HAS_LINE_INFO;
#endif /* JERRY_LINE_INFO */

  literal_pool_p = ((ecma_value_t *) byte_code_p) - context_p->register_count;
  byte_code_p += literal_length;
  dst_p = byte_code_p;

  parser_init_literal_pool (context_p, literal_pool_p);

  page_p = context_p->byte_code.first_p;
  offset = 0;
  real_offset = 0;
  uint8_t last_register_index = (uint8_t) JERRY_MIN (context_p->register_count,
                                                     (PARSER_MAXIMUM_NUMBER_OF_REGISTERS - 1));

  while (page_p != last_page_p || offset < last_position)
  {
    uint8_t flags;
    uint8_t *opcode_p;
    uint8_t *branch_mark_p;
    cbc_opcode_t opcode;
    size_t branch_offset_length;

    opcode_p = dst_p;
    branch_mark_p = page_p->bytes + offset;
    opcode = (cbc_opcode_t) (*branch_mark_p);
    branch_offset_length = CBC_BRANCH_OFFSET_LENGTH (opcode);

    if (opcode == CBC_JUMP_FORWARD)
    {
      /* These opcodes are deleted from the stream. */
      size_t counter = PARSER_MAX_BRANCH_LENGTH + 1;

      do
      {
        PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);
      }
      while (--counter > 0);

      continue;
    }

    /* Storing the opcode */
    *dst_p++ = (uint8_t) opcode;
    real_offset++;
    PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);
    flags = cbc_flags[opcode];

#if JERRY_DEBUGGER
    if (opcode == CBC_BREAKPOINT_DISABLED)
    {
      uint32_t bp_offset = (uint32_t) (((uint8_t *) dst_p) - ((uint8_t *) compiled_code_p) - 1);
      parser_append_breakpoint_info (context_p, JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST, bp_offset);
    }
#endif /* JERRY_DEBUGGER */

    if (opcode == CBC_EXT_OPCODE)
    {
      cbc_ext_opcode_t ext_opcode;

      ext_opcode = (cbc_ext_opcode_t) page_p->bytes[offset];
      flags = cbc_ext_flags[ext_opcode];
      branch_offset_length = CBC_BRANCH_OFFSET_LENGTH (ext_opcode);

      /* Storing the extended opcode */
      *dst_p++ = (uint8_t) ext_opcode;
      opcode_p++;
      real_offset++;
      PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);
    }

    /* Only literal and call arguments can be combined. */
    JERRY_ASSERT (!(flags & CBC_HAS_BRANCH_ARG)
                   || !(flags & (CBC_HAS_BYTE_ARG | CBC_HAS_LITERAL_ARG)));

    while (flags & (CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2))
    {
      uint16_t first_byte = page_p->bytes[offset];

      uint8_t *opcode_pos_p = dst_p - 1;
      *dst_p++ = (uint8_t) first_byte;
      real_offset++;
      PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);

      if (first_byte > literal_one_byte_limit)
      {
        *dst_p++ = page_p->bytes[offset];

        if (first_byte >= encoding_limit)
        {
          first_byte = (uint16_t) (((first_byte << 8) | dst_p[-1]) - encoding_delta);
        }
        real_offset++;
      }
      PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);

      if (flags & CBC_HAS_LITERAL_ARG2)
      {
        if (flags & CBC_HAS_LITERAL_ARG)
        {
          flags = CBC_HAS_LITERAL_ARG;
        }
        else
        {
          flags = CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2;
        }
      }
      else
      {
        if (opcode == CBC_ASSIGN_SET_IDENT && JERRY_LIKELY (first_byte < last_register_index))
        {
          *opcode_pos_p = CBC_MOV_IDENT;
        }

        break;
      }
    }

    if (flags & CBC_HAS_BYTE_ARG)
    {
      /* This argument will be copied without modification. */
      *dst_p++ = page_p->bytes[offset];
      real_offset++;
      PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);
      continue;
    }

    if (flags & CBC_HAS_BRANCH_ARG)
    {
      *branch_mark_p |= CBC_HIGHEST_BIT_MASK;
      bool prefix_zero = true;

      /* The leading zeroes are dropped from the stream. */
      JERRY_ASSERT (branch_offset_length > 0 && branch_offset_length <= 3);

      while (--branch_offset_length > 0)
      {
        uint8_t byte = page_p->bytes[offset];
        if (byte > 0 || !prefix_zero)
        {
          prefix_zero = false;
          *dst_p++ = page_p->bytes[offset];
          real_offset++;
        }
        else
        {
          /* When a leading zero is dropped, the branch
           * offset length must be decreased as well. */
          (*opcode_p)--;
        }
        PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);
      }

      *dst_p++ = page_p->bytes[offset];
      real_offset++;
      PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);
      continue;
    }
  }

#if JERRY_DEBUGGER
  if ((JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
      && context_p->breakpoint_info_count > 0)
  {
    parser_send_breakpoints (context_p, JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST);
    JERRY_ASSERT (context_p->breakpoint_info_count == 0);
  }
#endif /* JERRY_DEBUGGER */

  if (!(context_p->status_flags & PARSER_NO_END_LABEL))
  {
    *dst_p++ = CBC_RETURN_WITH_BLOCK;

#if JERRY_ESNEXT
    if (PARSER_IS_NORMAL_ASYNC_FUNCTION (context_p->status_flags))
    {
      dst_p[-1] = CBC_EXT_OPCODE;
      dst_p[0] = CBC_EXT_ASYNC_EXIT;
      dst_p++;
    }
#endif /* JERRY_ESNEXT */
  }
  JERRY_ASSERT (dst_p == byte_code_p + length);

#if JERRY_LINE_INFO
  uint8_t *line_info_p = parser_line_info_generate (context_p);
#endif /* JERRY_LINE_INFO */

  parse_update_branches (context_p, byte_code_p);

  parser_cbc_stream_free (&context_p->byte_code);

  if (context_p->status_flags & PARSER_HAS_LATE_LIT_INIT)
  {
    parser_list_iterator_t literal_iterator;
    lexer_literal_t *literal_p;
    uint16_t register_count = context_p->register_count;

    parser_list_iterator_init (&context_p->literal_pool, &literal_iterator);
    while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)))
    {
      if ((literal_p->status_flags & LEXER_FLAG_LATE_INIT)
          && literal_p->prop.index >= register_count)
      {
        uint32_t source_data = literal_p->u.source_data;
        const uint8_t *char_p = context_p->source_end_p - (source_data & 0xfffff);
        ecma_value_t lit_value = ecma_find_or_create_literal_string (char_p,
                                                                     source_data >> 20);
        literal_pool_p[literal_p->prop.index] = lit_value;
      }
    }
  }

  ecma_value_t *base_p = (ecma_value_t *) (((uint8_t *) compiled_code_p) + total_size);

  if (PARSER_NEEDS_MAPPED_ARGUMENTS (context_p->status_flags))
  {
    parser_list_iterator_t literal_iterator;
    uint16_t argument_count = 0;
    uint16_t register_count = context_p->register_count;
    base_p -= context_p->argument_count;

    parser_list_iterator_init (&context_p->literal_pool, &literal_iterator);
    while (argument_count < context_p->argument_count)
    {
      lexer_literal_t *literal_p;
      literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator);

      JERRY_ASSERT (literal_p != NULL);

      if (!(literal_p->status_flags & LEXER_FLAG_FUNCTION_ARGUMENT))
      {
        continue;
      }

      /* All arguments must be moved to initialized registers. */
      if (literal_p->type == LEXER_UNUSED_LITERAL)
      {
        base_p[argument_count] = ECMA_VALUE_EMPTY;
        argument_count++;
        continue;
      }

      JERRY_ASSERT (literal_p->type == LEXER_IDENT_LITERAL);

      JERRY_ASSERT (literal_p->prop.index >= register_count);

      base_p[argument_count] = literal_pool_p[literal_p->prop.index];
      argument_count++;
    }
  }

#if JERRY_ESNEXT
  if (!(context_p->status_flags & PARSER_CLASS_CONSTRUCTOR))
  {
    *(--base_p) = ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  if (context_p->argument_length != UINT16_MAX)
  {
    compiled_code_p->status_flags |= CBC_CODE_FLAGS_HAS_EXTENDED_INFO;
    *(--base_p) = context_p->argument_length;
  }

  if (context_p->tagged_template_literal_cp != JMEM_CP_NULL)
  {
    compiled_code_p->status_flags |= CBC_CODE_FLAGS_HAS_TAGGED_LITERALS;
    base_p[-1] = (ecma_value_t) context_p->tagged_template_literal_cp;
#if JERRY_LINE_INFO
    --base_p;
#endif /* JERRY_LINE_INFO */
  }
#endif /* JERRY_ESNEXT */

#if JERRY_LINE_INFO
  ECMA_SET_INTERNAL_VALUE_POINTER (base_p[-1], line_info_p);
#endif /* JERRY_LINE_INFO */

#if JERRY_PARSER_DUMP_BYTE_CODE
  if (context_p->is_show_opcodes)
  {
    util_print_cbc (compiled_code_p);
    JERRY_DEBUG_MSG ("\nByte code size: %d bytes\n", (int) length);
    context_p->total_byte_code_size += (uint32_t) length;
  }
#endif /* JERRY_PARSER_DUMP_BYTE_CODE */

#if JERRY_DEBUGGER
  if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
  {
    jerry_debugger_send_function_cp (JERRY_DEBUGGER_BYTE_CODE_CP, compiled_code_p);
  }
#endif /* JERRY_DEBUGGER */

  return compiled_code_p;
} /* parser_post_processing */