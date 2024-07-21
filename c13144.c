lexer_parse_identifier (parser_context_t *context_p, /**< context */
                        lexer_parse_options_t options) /**< check keywords */
{
  /* Only very few identifiers contains \u escape sequences. */
  const uint8_t *source_p = context_p->source_p;
  /* Note: newline or tab cannot be part of an identifier. */
  parser_line_counter_t column = context_p->column;
  const uint8_t *source_end_p = context_p->source_end_p;
  size_t length = 0;
  uint8_t has_escape = false;

  do
  {
    if (*source_p == LIT_CHAR_BACKSLASH)
    {
      /* After a backslash an identifier must start. */
      lit_code_point_t code_point = UINT32_MAX;
      uint32_t escape_length = 6;

      if (options & (LEXER_PARSE_CHECK_START_AND_RETURN | LEXER_PARSE_CHECK_PART_AND_RETURN))
      {
        return true;
      }

      has_escape = true;

#if JERRY_ESNEXT
      if (source_p + 5 <= source_end_p && source_p[1] == LIT_CHAR_LOWERCASE_U)
      {
        if (source_p[2] == LIT_CHAR_LEFT_BRACE)
        {
          code_point = lexer_hex_in_braces_to_code_point (source_p + 3, source_end_p, &escape_length);
        }
        else if (source_p + 6 <= source_end_p)
        {
          code_point = lexer_hex_to_code_point (source_p + 2, 4);
        }
      }
#else /* !JERRY_ESNEXT */
      if (source_p + 6 <= source_end_p && source_p[1] == LIT_CHAR_LOWERCASE_U)
      {
        code_point = lexer_hex_to_code_point (source_p + 2, 4);
      }
#endif /* JERRY_ESNEXT */

      if (code_point == UINT32_MAX)
      {
        context_p->source_p = source_p;
        context_p->token.column = column;
        parser_raise_error (context_p, PARSER_ERR_INVALID_UNICODE_ESCAPE_SEQUENCE);
      }

      if (length == 0)
      {
        if (!lit_code_point_is_identifier_start (code_point))
        {
          parser_raise_error (context_p, PARSER_ERR_INVALID_IDENTIFIER_START);
        }
      }
      else
      {
        if (!lit_code_point_is_identifier_part (code_point))
        {
          parser_raise_error (context_p, PARSER_ERR_INVALID_IDENTIFIER_PART);
        }
      }

      length += lit_code_point_get_cesu8_length (code_point);
      source_p += escape_length;
      PARSER_PLUS_EQUAL_LC (column, escape_length);
      continue;
    }

    lit_code_point_t code_point = *source_p;
    lit_utf8_size_t utf8_length = 1, decoded_length = 1, char_count = 1;

    if (JERRY_UNLIKELY (code_point >= LIT_UTF8_2_BYTE_MARKER))
    {
#if JERRY_ESNEXT
      utf8_length = lit_read_code_point_from_utf8 (source_p,
                                                   (lit_utf8_size_t) (source_end_p - source_p),
                                                   &code_point);
      decoded_length = utf8_length;

      /* Only ES2015+ supports code points outside of the basic plane which can be part of an identifier. */
      if ((code_point >= LIT_UTF16_HIGH_SURROGATE_MIN && code_point <= LIT_UTF16_HIGH_SURROGATE_MAX)
          && source_p + 3 < source_end_p)
      {
        lit_code_point_t low_surrogate;
        lit_read_code_point_from_utf8 (source_p + 3,
                                       (lit_utf8_size_t) (source_end_p - (source_p + 3)),
                                       &low_surrogate);

        if (low_surrogate >= LIT_UTF16_LOW_SURROGATE_MIN && low_surrogate <= LIT_UTF16_LOW_SURROGATE_MAX)
        {
          code_point = lit_convert_surrogate_pair_to_code_point ((ecma_char_t) code_point,
                                                                 (ecma_char_t) low_surrogate);
          utf8_length = 2 * 3;
          decoded_length = 2 * 3;
          char_count = 2;
        }
      }
      else if (source_p[0] >= LIT_UTF8_4_BYTE_MARKER)
      {
        decoded_length = 2 * 3;
        has_escape = true;
      }
#else /* !JERRY_ESNEXT */
      if (code_point < LIT_UTF8_4_BYTE_MARKER)
      {
        utf8_length = lit_read_code_point_from_utf8 (source_p,
                                                     (lit_utf8_size_t) (source_end_p - source_p),
                                                     &code_point);
        decoded_length = utf8_length;
      }
      else
      {
        code_point = 0;
      }
#endif /* JERRY_ESNEXT */
    }

    if (length == 0)
    {
      if (JERRY_UNLIKELY (options & (LEXER_PARSE_CHECK_START_AND_RETURN | LEXER_PARSE_CHECK_PART_AND_RETURN)))
      {
        if (options & LEXER_PARSE_CHECK_START_AND_RETURN)
        {
          return lit_code_point_is_identifier_start (code_point);
        }
        else
        {
          return lit_code_point_is_identifier_part (code_point);
        }
      }

      if (!lit_code_point_is_identifier_start (code_point))
      {
        return false;
      }
    }
    else if (!lit_code_point_is_identifier_part (code_point))
    {
      break;
    }

    source_p += utf8_length;
    length += decoded_length;
    PARSER_PLUS_EQUAL_LC (column, char_count);
  }
  while (source_p < source_end_p);

  JERRY_ASSERT (length > 0);

  context_p->token.type = LEXER_LITERAL;
  context_p->token.lit_location.type = LEXER_IDENT_LITERAL;
  context_p->token.lit_location.has_escape = has_escape;

  context_p->token.column = context_p->column;
  context_p->token.lit_location.char_p = context_p->source_p;
  context_p->token.lit_location.length = (prop_length_t) length;

  if (JERRY_UNLIKELY (length > PARSER_MAXIMUM_IDENT_LENGTH))
  {
    parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_TOO_LONG);
  }

  /* Check keywords. */
  if ((options & LEXER_PARSE_CHECK_KEYWORDS)
      && (length >= LEXER_KEYWORD_MIN_LENGTH && length <= LEXER_KEYWORD_MAX_LENGTH))
  {
    const uint8_t *ident_start_p = context_p->source_p;
    uint8_t buffer_p[LEXER_KEYWORD_MAX_LENGTH];

    if (JERRY_UNLIKELY (context_p->token.lit_location.has_escape))
    {
      lexer_convert_ident_to_cesu8 (buffer_p, ident_start_p, (prop_length_t) length);
      ident_start_p = buffer_p;
    }

    const keyword_string_t *keyword_list_p = keyword_strings_list[length - LEXER_KEYWORD_MIN_LENGTH];

    int start = 0;
    int end = keyword_lengths_list[length - LEXER_KEYWORD_MIN_LENGTH];
    int middle = end / 2;

    do
    {
      const keyword_string_t *keyword_p = keyword_list_p + middle;
      int compare_result = ident_start_p[0] - keyword_p->keyword_p[0];

      if (compare_result == 0)
      {
        compare_result = memcmp (ident_start_p, keyword_p->keyword_p, length);

        if (compare_result == 0)
        {
          context_p->token.keyword_type = (uint8_t) keyword_p->type;

          if (JERRY_LIKELY (keyword_p->type < LEXER_FIRST_NON_RESERVED_KEYWORD))
          {
#if JERRY_ESNEXT
            if (JERRY_UNLIKELY (keyword_p->type == LEXER_KEYW_AWAIT))
            {
              if (!(context_p->status_flags & PARSER_IS_ASYNC_FUNCTION)
                  && !(context_p->global_status_flags & ECMA_PARSE_MODULE))
              {
                break;
              }

              if (context_p->status_flags & PARSER_DISALLOW_AWAIT_YIELD)
              {
                if (LEXER_CHECK_INVALID_KEYWORD (ident_start_p, buffer_p))
                {
                  parser_raise_error (context_p, PARSER_ERR_INVALID_KEYWORD);
                }
                parser_raise_error (context_p, PARSER_ERR_AWAIT_NOT_ALLOWED);
              }

              context_p->token.type = (uint8_t) LEXER_KEYW_AWAIT;
              break;
            }
#endif /* JERRY_ESNEXT */

            if (LEXER_CHECK_INVALID_KEYWORD (ident_start_p, buffer_p))
            {
              /* Escape sequences are not allowed in a keyword. */
              parser_raise_error (context_p, PARSER_ERR_INVALID_KEYWORD);
            }

            context_p->token.type = (uint8_t) keyword_p->type;
            break;
          }

#if JERRY_ESNEXT
          if (keyword_p->type == LEXER_KEYW_LET && (context_p->status_flags & PARSER_IS_STRICT))
          {
            if (LEXER_CHECK_INVALID_KEYWORD (ident_start_p, buffer_p))
            {
              parser_raise_error (context_p, PARSER_ERR_INVALID_KEYWORD);
            }

            context_p->token.type = (uint8_t) LEXER_KEYW_LET;
            break;
          }

          if (keyword_p->type == LEXER_KEYW_YIELD && (context_p->status_flags & PARSER_IS_GENERATOR_FUNCTION))
          {
            if (context_p->status_flags & PARSER_DISALLOW_AWAIT_YIELD)
            {
              if (LEXER_CHECK_INVALID_KEYWORD (ident_start_p, buffer_p))
              {
                parser_raise_error (context_p, PARSER_ERR_INVALID_KEYWORD);
              }
              parser_raise_error (context_p, PARSER_ERR_YIELD_NOT_ALLOWED);
            }

            context_p->token.type = (uint8_t) LEXER_KEYW_YIELD;
            break;
          }

          if (keyword_p->type == LEXER_KEYW_ARGUMENTS && (context_p->status_flags & PARSER_INSIDE_CLASS_FIELD))
          {
            parser_raise_error (context_p, PARSER_ERR_ARGUMENTS_IN_CLASS_FIELD);
          }
#endif /* JERRY_ESNEXT */

          if (keyword_p->type >= LEXER_FIRST_FUTURE_STRICT_RESERVED_WORD
              && (context_p->status_flags & PARSER_IS_STRICT))
          {
            parser_raise_error (context_p, PARSER_ERR_STRICT_IDENT_NOT_ALLOWED);
          }
          break;
        }
      }

      if (compare_result > 0)
      {
        start = middle + 1;
      }
      else
      {
        JERRY_ASSERT (compare_result < 0);
        end = middle;
      }

      middle = (start + end) / 2;
    }
    while (start < end);
  }

  context_p->source_p = source_p;
  context_p->column = column;
  return true;
} /* lexer_parse_identifier */