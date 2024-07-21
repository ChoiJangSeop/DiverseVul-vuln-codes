scanner_check_variables (parser_context_t *context_p) /**< context */
{
  scanner_info_t *info_p = context_p->next_scanner_info_p;
  const uint8_t *next_data_p = (const uint8_t *) (info_p + 1);
  lexer_lit_location_t literal;

  JERRY_ASSERT (info_p->type == SCANNER_TYPE_FUNCTION);

  literal.char_p = info_p->source_p - 1;

  while (next_data_p[0] != SCANNER_STREAM_TYPE_END)
  {
    uint32_t type = next_data_p[0] & SCANNER_STREAM_TYPE_MASK;
    const uint8_t *data_p = next_data_p;

    JERRY_ASSERT (type != SCANNER_STREAM_TYPE_HOLE
                  && !SCANNER_STREAM_TYPE_IS_ARG (type)
                  && !SCANNER_STREAM_TYPE_IS_ARG_FUNC (type));
    JERRY_ASSERT (data_p[0] & SCANNER_STREAM_NO_REG);

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

    literal.length = data_p[1];
    literal.type = LEXER_IDENT_LITERAL;
    literal.has_escape = (data_p[0] & SCANNER_STREAM_HAS_ESCAPE) ? 1 : 0;

    lexer_construct_literal_object (context_p, &literal, LEXER_NEW_IDENT_LITERAL);
    literal.char_p += data_p[1];

#if JERRY_MODULE_SYSTEM
    if (type == SCANNER_STREAM_TYPE_IMPORT)
    {
      continue;
    }
#endif /* JERRY_MODULE_SYSTEM */

    context_p->lit_object.literal_p->status_flags |= LEXER_FLAG_USED;

    uint16_t opcode;
    if (type == SCANNER_STREAM_TYPE_VAR || type == SCANNER_STREAM_TYPE_FUNC)
    {
      opcode = CBC_CHECK_VAR;
    }
    else
    {
      opcode = CBC_CHECK_LET;
    }

    parser_emit_cbc_literal (context_p, opcode, context_p->lit_object.index);
  }

  parser_flush_cbc (context_p);
} /* scanner_check_variables */