parser_compute_indicies (parser_context_t *context_p, /**< context */
                         uint16_t *ident_end, /**< end of the identifier group */
                         uint16_t *const_literal_end) /**< end of the const literal group */
{
  parser_list_iterator_t literal_iterator;
  lexer_literal_t *literal_p;

  uint16_t ident_count = 0;
  uint16_t const_literal_count = 0;

  uint16_t ident_index;
  uint16_t const_literal_index;
  uint16_t literal_index;

  /* First phase: count the number of items in each group. */
  parser_list_iterator_init (&context_p->literal_pool, &literal_iterator);
  while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)))
  {
    switch (literal_p->type)
    {
      case LEXER_IDENT_LITERAL:
      {
        if (literal_p->status_flags & LEXER_FLAG_USED)
        {
          ident_count++;
          break;
        }
        else if (!(literal_p->status_flags & LEXER_FLAG_SOURCE_PTR))
        {
          jmem_heap_free_block ((void *) literal_p->u.char_p, literal_p->prop.length);
          /* This literal should not be freed even if an error is encountered later. */
          literal_p->status_flags |= LEXER_FLAG_SOURCE_PTR;
        }
        continue;
      }
      case LEXER_STRING_LITERAL:
      {
        const_literal_count++;
        break;
      }
      case LEXER_NUMBER_LITERAL:
      {
        const_literal_count++;
        continue;
      }
      case LEXER_FUNCTION_LITERAL:
      case LEXER_REGEXP_LITERAL:
      {
        continue;
      }
      default:
      {
        JERRY_ASSERT (literal_p->type == LEXER_UNUSED_LITERAL);
        continue;
      }
    }

    const uint8_t *char_p = literal_p->u.char_p;
    uint32_t status_flags = context_p->status_flags;

    if ((literal_p->status_flags & LEXER_FLAG_SOURCE_PTR)
        && literal_p->prop.length < 0xfff)
    {
      size_t bytes_to_end = (size_t) (context_p->source_end_p - char_p);

      if (bytes_to_end < 0xfffff)
      {
        literal_p->u.source_data = ((uint32_t) bytes_to_end) | (((uint32_t) literal_p->prop.length) << 20);
        literal_p->status_flags |= LEXER_FLAG_LATE_INIT;
        status_flags |= PARSER_HAS_LATE_LIT_INIT;
        context_p->status_flags = status_flags;
        char_p = NULL;
      }
    }

    if (char_p != NULL)
    {
      literal_p->u.value = ecma_find_or_create_literal_string (char_p,
                                                               literal_p->prop.length);

      if (!(literal_p->status_flags & LEXER_FLAG_SOURCE_PTR))
      {
        jmem_heap_free_block ((void *) char_p, literal_p->prop.length);
        /* This literal should not be freed even if an error is encountered later. */
        literal_p->status_flags |= LEXER_FLAG_SOURCE_PTR;
      }
    }
  }

  ident_index = context_p->register_count;
  const_literal_index = (uint16_t) (ident_index + ident_count);
  literal_index = (uint16_t) (const_literal_index + const_literal_count);

  /* Second phase: Assign an index to each literal. */
  parser_list_iterator_init (&context_p->literal_pool, &literal_iterator);

  while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)))
  {
    switch (literal_p->type)
    {
      case LEXER_IDENT_LITERAL:
      {
        if (literal_p->status_flags & LEXER_FLAG_USED)
        {
          literal_p->prop.index = ident_index;
          ident_index++;
        }
        break;
      }
      case LEXER_STRING_LITERAL:
      case LEXER_NUMBER_LITERAL:
      {
        JERRY_ASSERT ((literal_p->status_flags & ~(LEXER_FLAG_SOURCE_PTR | LEXER_FLAG_LATE_INIT)) == 0);
        literal_p->prop.index = const_literal_index;
        const_literal_index++;
        break;
      }
      case LEXER_FUNCTION_LITERAL:
      case LEXER_REGEXP_LITERAL:
      {
        JERRY_ASSERT (literal_p->status_flags == 0);

        literal_p->prop.index = literal_index;
        literal_index++;
        break;
      }
      default:
      {
        JERRY_ASSERT (literal_p->type == LEXER_UNUSED_LITERAL
                      && literal_p->status_flags == LEXER_FLAG_FUNCTION_ARGUMENT);
        break;
      }
    }
  }

  JERRY_ASSERT (ident_index == context_p->register_count + ident_count);
  JERRY_ASSERT (const_literal_index == ident_index + const_literal_count);
  JERRY_ASSERT (literal_index <= context_p->register_count + context_p->literal_count);

  context_p->literal_count = literal_index;

  *ident_end = ident_index;
  *const_literal_end = const_literal_index;
} /* parser_compute_indicies */