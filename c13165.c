lexer_parse_string (parser_context_t *context_p, /**< context */
                    lexer_string_options_t opts) /**< options */
{
#if JERRY_ESNEXT
  int32_t raw_length_adjust = 0;
#else /* JERRY_ESNEXT */
  JERRY_UNUSED (opts);
#endif /* JERRY_ESNEXT */

  uint8_t str_end_character = context_p->source_p[0];
  const uint8_t *source_p = context_p->source_p + 1;
  const uint8_t *string_start_p = source_p;
  const uint8_t *source_end_p = context_p->source_end_p;
  parser_line_counter_t line = context_p->line;
  parser_line_counter_t column = (parser_line_counter_t) (context_p->column + 1);
  parser_line_counter_t original_line = line;
  parser_line_counter_t original_column = column;
  size_t length = 0;
  uint8_t has_escape = false;

#if JERRY_ESNEXT
  if (str_end_character == LIT_CHAR_RIGHT_BRACE)
  {
    str_end_character = LIT_CHAR_GRAVE_ACCENT;
  }
#endif /* JERRY_ESNEXT */

  while (true)
  {
    if (source_p >= source_end_p)
    {
      context_p->token.line = original_line;
      context_p->token.column = (parser_line_counter_t) (original_column - 1);
      parser_raise_error (context_p, PARSER_ERR_UNTERMINATED_STRING);
    }

    if (*source_p == str_end_character)
    {
      break;
    }

    if (*source_p == LIT_CHAR_BACKSLASH)
    {
      source_p++;
      column++;
      if (source_p >= source_end_p)
      {
        /* Will throw an unterminated string error. */
        continue;
      }

      has_escape = true;

      /* Newline is ignored. */
      if (*source_p == LIT_CHAR_CR)
      {
        source_p++;
        if (source_p < source_end_p
            && *source_p == LIT_CHAR_LF)
        {
#if JERRY_ESNEXT
          raw_length_adjust--;
#endif /* JERRY_ESNEXT */
          source_p++;
        }

        line++;
        column = 1;
        continue;
      }
      else if (*source_p == LIT_CHAR_LF)
      {
        source_p++;
        line++;
        column = 1;
        continue;
      }
      else if (*source_p == LEXER_NEWLINE_LS_PS_BYTE_1 && LEXER_NEWLINE_LS_PS_BYTE_23 (source_p))
      {
        source_p += 3;
        line++;
        column = 1;
        continue;
      }

#if JERRY_ESNEXT
      if (opts & LEXER_STRING_RAW)
      {
        if ((*source_p == LIT_CHAR_GRAVE_ACCENT) || (*source_p == LIT_CHAR_BACKSLASH))
        {
          source_p++;
          column++;
          length++;
        }
        continue;
      }
#endif /* JERRY_ESNEXT */

      if (*source_p == LIT_CHAR_0
          && source_p + 1 < source_end_p
          && (*(source_p + 1) < LIT_CHAR_0 || *(source_p + 1) > LIT_CHAR_9))
      {
        source_p++;
        column++;
        length++;
        continue;
      }

      /* Except \x, \u, and octal numbers, everything is
       * converted to a character which has the same byte length. */
      if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_3)
      {
#if JERRY_ESNEXT
        if (str_end_character == LIT_CHAR_GRAVE_ACCENT)
        {
          parser_raise_error (context_p, PARSER_ERR_TEMPLATE_STR_OCTAL_ESCAPE);
        }
#endif

        if (context_p->status_flags & PARSER_IS_STRICT)
        {
          parser_raise_error (context_p, PARSER_ERR_OCTAL_ESCAPE_NOT_ALLOWED);
        }

        source_p++;
        column++;

        if (source_p < source_end_p && *source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
        {
          source_p++;
          column++;

          if (source_p < source_end_p && *source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
          {
            /* Numbers >= 0x200 (0x80) requires
             * two bytes for encoding in UTF-8. */
            if (source_p[-2] >= LIT_CHAR_2)
            {
              length++;
            }

            source_p++;
            column++;
          }
        }

        length++;
        continue;
      }

      if (*source_p >= LIT_CHAR_4 && *source_p <= LIT_CHAR_7)
      {
        if (context_p->status_flags & PARSER_IS_STRICT)
        {
          parser_raise_error (context_p, PARSER_ERR_OCTAL_ESCAPE_NOT_ALLOWED);
        }

        source_p++;
        column++;

        if (source_p < source_end_p && *source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
        {
          source_p++;
          column++;
        }

        /* The maximum number is 0x4d so the UTF-8
         * representation is always one byte. */
        length++;
        continue;
      }

      if (*source_p == LIT_CHAR_LOWERCASE_X || *source_p == LIT_CHAR_LOWERCASE_U)
      {
        uint32_t escape_length = (*source_p == LIT_CHAR_LOWERCASE_X) ? 3 : 5;
        lit_code_point_t code_point = UINT32_MAX;

#if JERRY_ESNEXT
        if (source_p + 4 <= source_end_p
            && source_p[0] == LIT_CHAR_LOWERCASE_U
            && source_p[1] == LIT_CHAR_LEFT_BRACE)
        {
          code_point = lexer_hex_in_braces_to_code_point (source_p + 2, source_end_p, &escape_length);
          escape_length--;
        }
        else
        {
#endif /* JERRY_ESNEXT */
          if (source_p + escape_length <= source_end_p)
          {
            code_point = lexer_hex_to_code_point (source_p + 1, escape_length - 1);
          }
#if JERRY_ESNEXT
        }
#endif /* JERRY_ESNEXT */

        if (code_point == UINT32_MAX)
        {
          context_p->token.line = line;
          context_p->token.column = (parser_line_counter_t) (column - 1);
          parser_raise_error (context_p, PARSER_ERR_INVALID_UNICODE_ESCAPE_SEQUENCE);
        }

        length += lit_code_point_get_cesu8_length (code_point);

        source_p += escape_length;
        PARSER_PLUS_EQUAL_LC (column, escape_length);
        continue;
      }
    }
#if JERRY_ESNEXT
    else if (str_end_character == LIT_CHAR_GRAVE_ACCENT &&
             source_p[0] == LIT_CHAR_DOLLAR_SIGN &&
             source_p + 1 < source_end_p &&
             source_p[1] == LIT_CHAR_LEFT_BRACE)
    {
      raw_length_adjust--;
      source_p++;
      break;
    }
#endif /* JERRY_ESNEXT */

    if (*source_p >= LIT_UTF8_4_BYTE_MARKER)
    {
      /* Processing 4 byte unicode sequence (even if it is
       * after a backslash). Always converted to two 3 byte
       * long sequence. */
      length += 2 * 3;
      has_escape = true;
      source_p += 4;
#if JERRY_ESNEXT
      raw_length_adjust += 2;
#endif /* JERRY_ESNEXT */
      column++;
      continue;
    }
    else if (*source_p == LIT_CHAR_TAB)
    {
      column = align_column_to_tab (column);
      /* Subtract -1 because column is increased below. */
      column--;
    }
#if JERRY_ESNEXT
    else if (*source_p == LEXER_NEWLINE_LS_PS_BYTE_1 && LEXER_NEWLINE_LS_PS_BYTE_23 (source_p))
    {
      source_p += 3;
      length += 3;
      line++;
      column = 1;
      continue;
    }
    else if (str_end_character == LIT_CHAR_GRAVE_ACCENT)
    {
      /* Newline (without backslash) is part of the string.
         Note: ECMAScript v6, 11.8.6.1 <CR> or <CR><LF> are both normalized to <LF> */
      if (*source_p == LIT_CHAR_CR)
      {
        has_escape = true;
        source_p++;
        length++;
        if (source_p < source_end_p
            && *source_p == LIT_CHAR_LF)
        {
          source_p++;
          raw_length_adjust--;
        }
        line++;
        column = 1;
        continue;
      }
      else if (*source_p == LIT_CHAR_LF)
      {
        source_p++;
        length++;
        line++;
        column = 1;
        continue;
      }
    }
#endif /* JERRY_ESNEXT */
    else if (*source_p == LIT_CHAR_CR
#if !JERRY_ESNEXT
             || (*source_p == LEXER_NEWLINE_LS_PS_BYTE_1 && LEXER_NEWLINE_LS_PS_BYTE_23 (source_p))
#endif /* !JERRY_ESNEXT */
             || *source_p == LIT_CHAR_LF)
    {
      context_p->token.line = line;
      context_p->token.column = column;
      parser_raise_error (context_p, PARSER_ERR_NEWLINE_NOT_ALLOWED);
    }

    source_p++;
    column++;
    length++;

    while (source_p < source_end_p
           && IS_UTF8_INTERMEDIATE_OCTET (*source_p))
    {
      source_p++;
      length++;
    }
  }

#if JERRY_ESNEXT
  if (opts & LEXER_STRING_RAW)
  {
    length = (size_t) ((source_p - string_start_p) + raw_length_adjust);
  }
#endif /* JERRY_ESNEXT */

  if (length > PARSER_MAXIMUM_STRING_LENGTH)
  {
    parser_raise_error (context_p, PARSER_ERR_STRING_TOO_LONG);
  }

#if JERRY_ESNEXT
  context_p->token.type = ((str_end_character != LIT_CHAR_GRAVE_ACCENT) ? LEXER_LITERAL
                                                                        : LEXER_TEMPLATE_LITERAL);
#else /* !JERRY_ESNEXT */
  context_p->token.type = LEXER_LITERAL;
#endif /* JERRY_ESNEXT */

  /* Fill literal data. */
  context_p->token.lit_location.char_p = string_start_p;
  context_p->token.lit_location.length = (prop_length_t) length;
  context_p->token.lit_location.type = LEXER_STRING_LITERAL;
  context_p->token.lit_location.has_escape = has_escape;

  context_p->source_p = source_p + 1;
  context_p->line = line;
  context_p->column = (parser_line_counter_t) (column + 1);
} /* lexer_parse_string */