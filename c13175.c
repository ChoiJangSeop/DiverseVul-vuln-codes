lexer_construct_literal_object (parser_context_t *context_p, /**< context */
                                const lexer_lit_location_t *lit_location_p, /**< literal location */
                                uint8_t literal_type) /**< final literal type */
{
  uint8_t local_byte_array[LEXER_MAX_LITERAL_LOCAL_BUFFER_SIZE];

  const uint8_t *char_p = lexer_convert_literal_to_chars (context_p,
                                                          lit_location_p,
                                                          local_byte_array,
                                                          LEXER_STRING_NO_OPTS);

  size_t length = lit_location_p->length;
  parser_list_iterator_t literal_iterator;
  lexer_literal_t *literal_p;
  uint32_t literal_index = 0;
  bool search_scope_stack = (literal_type == LEXER_IDENT_LITERAL);

  if (JERRY_UNLIKELY (literal_type == LEXER_NEW_IDENT_LITERAL))
  {
    literal_type = LEXER_IDENT_LITERAL;
  }

  JERRY_ASSERT (literal_type == LEXER_IDENT_LITERAL
                || literal_type == LEXER_STRING_LITERAL);

  JERRY_ASSERT (literal_type != LEXER_IDENT_LITERAL || length <= PARSER_MAXIMUM_IDENT_LENGTH);
  JERRY_ASSERT (literal_type != LEXER_STRING_LITERAL || length <= PARSER_MAXIMUM_STRING_LENGTH);

  parser_list_iterator_init (&context_p->literal_pool, &literal_iterator);

  while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
  {
    if (literal_p->type == literal_type
        && literal_p->prop.length == length
        && memcmp (literal_p->u.char_p, char_p, length) == 0)
    {
      context_p->lit_object.literal_p = literal_p;
      context_p->lit_object.index = (uint16_t) literal_index;

      parser_free_allocated_buffer (context_p);

      if (search_scope_stack)
      {
        parser_scope_stack_t *scope_stack_start_p = context_p->scope_stack_p;
        parser_scope_stack_t *scope_stack_p = scope_stack_start_p + context_p->scope_stack_top;

        while (scope_stack_p > scope_stack_start_p)
        {
          scope_stack_p--;

          if (scope_stack_p->map_from == literal_index)
          {
            JERRY_ASSERT (scanner_decode_map_to (scope_stack_p) >= PARSER_REGISTER_START
                          || (literal_p->status_flags & LEXER_FLAG_USED));
            context_p->lit_object.index = scanner_decode_map_to (scope_stack_p);
            return;
          }
        }

        literal_p->status_flags |= LEXER_FLAG_USED;
      }
      return;
    }

    literal_index++;
  }

  JERRY_ASSERT (literal_index == context_p->literal_count);

  if (literal_index >= PARSER_MAXIMUM_NUMBER_OF_LITERALS)
  {
    parser_raise_error (context_p, PARSER_ERR_LITERAL_LIMIT_REACHED);
  }

  literal_p = (lexer_literal_t *) parser_list_append (context_p, &context_p->literal_pool);
  literal_p->prop.length = (prop_length_t) length;
  literal_p->type = literal_type;

  uint8_t status_flags = LEXER_FLAG_SOURCE_PTR;

  if (length > 0 && char_p == local_byte_array)
  {
    literal_p->u.char_p = (uint8_t *) jmem_heap_alloc_block (length);
    memcpy ((uint8_t *) literal_p->u.char_p, char_p, length);
    status_flags = 0;
  }
  else
  {
    literal_p->u.char_p = char_p;

    /* Buffer is taken over when a new literal is constructed. */
    if (context_p->u.allocated_buffer_p != NULL)
    {
      JERRY_ASSERT (char_p == context_p->u.allocated_buffer_p);

      context_p->u.allocated_buffer_p = NULL;
      status_flags = 0;
    }
  }

  if (search_scope_stack)
  {
    status_flags |= LEXER_FLAG_USED;
  }

  literal_p->status_flags = status_flags;

  context_p->lit_object.literal_p = literal_p;
  context_p->lit_object.index = (uint16_t) literal_index;
  context_p->literal_count++;

  JERRY_ASSERT (context_p->u.allocated_buffer_p == NULL);
} /* lexer_construct_literal_object */