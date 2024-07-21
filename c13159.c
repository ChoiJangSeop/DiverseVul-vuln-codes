lexer_parse_number (parser_context_t *context_p) /**< context */
{
  const uint8_t *source_p = context_p->source_p;
  const uint8_t *source_end_p = context_p->source_end_p;
  bool can_be_float = false;
#if JERRY_BUILTIN_BIGINT
  bool can_be_bigint = true;
#endif /* JERRY_BUILTIN_BIGINT */
  size_t length;

  context_p->token.type = LEXER_LITERAL;
  context_p->token.extra_value = LEXER_NUMBER_DECIMAL;
  context_p->token.lit_location.char_p = source_p;
  context_p->token.lit_location.type = LEXER_NUMBER_LITERAL;
  context_p->token.lit_location.has_escape = false;

  if (source_p[0] == LIT_CHAR_0
      && source_p + 1 < source_end_p)
  {
#if JERRY_ESNEXT
    if (source_p[1] == LIT_CHAR_UNDERSCORE)
    {
      parser_raise_error (context_p, PARSER_ERR_INVALID_UNDERSCORE_IN_NUMBER);
    }
#endif /* JERRY_ESNEXT */
    if (LEXER_TO_ASCII_LOWERCASE (source_p[1]) == LIT_CHAR_LOWERCASE_X)
    {
      context_p->token.extra_value = LEXER_NUMBER_HEXADECIMAL;
      source_p += 2;

      if (source_p >= source_end_p
          || !lit_char_is_hex_digit (source_p[0]))
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_HEX_DIGIT);
      }

      do
      {
        source_p++;
#if JERRY_ESNEXT
        if (source_p < source_end_p && source_p[0] == LIT_CHAR_UNDERSCORE)
        {
          source_p++;
          if (source_p == source_end_p || !lit_char_is_hex_digit (source_p[0]))
          {
            parser_raise_error (context_p, PARSER_ERR_INVALID_UNDERSCORE_IN_NUMBER);
          }
        }
#endif /* JERRY_ESNEXT */
      }
      while (source_p < source_end_p
             && lit_char_is_hex_digit (source_p[0]));
    }
#if JERRY_ESNEXT
    else if (LEXER_TO_ASCII_LOWERCASE (source_p[1]) == LIT_CHAR_LOWERCASE_O)
    {
      context_p->token.extra_value = LEXER_NUMBER_OCTAL;
      source_p += 2;

      if (source_p >= source_end_p
          || !lit_char_is_octal_digit (source_p[0]))
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_OCTAL_DIGIT);
      }

      lexer_check_numbers (context_p, &source_p, source_end_p, LIT_CHAR_7, false);
    }
#endif /* JERRY_ESNEXT */
    else if (source_p[1] >= LIT_CHAR_0
             && source_p[1] <= LIT_CHAR_9)
    {
      context_p->token.extra_value = LEXER_NUMBER_OCTAL;
#if JERRY_BUILTIN_BIGINT
      can_be_bigint = false;
#endif /* JERRY_BUILTIN_BIGINT */

      if (context_p->status_flags & PARSER_IS_STRICT)
      {
        parser_raise_error (context_p, PARSER_ERR_OCTAL_NUMBER_NOT_ALLOWED);
      }

      lexer_check_numbers (context_p, &source_p, source_end_p, LIT_CHAR_7, true);

      if (source_p < source_end_p
          && source_p[0] >= LIT_CHAR_8
          && source_p[0] <= LIT_CHAR_9)
      {
#if JERRY_ESNEXT
        lexer_check_numbers (context_p, &source_p, source_end_p, LIT_CHAR_9, true);
        context_p->token.extra_value = LEXER_NUMBER_DECIMAL;
#else /* !JERRY_ESNEXT */
        parser_raise_error (context_p, PARSER_ERR_INVALID_NUMBER);
#endif /* JERRY_ESNEXT */
      }
    }
#if JERRY_ESNEXT
    else if (LEXER_TO_ASCII_LOWERCASE (source_p[1]) == LIT_CHAR_LOWERCASE_B)
    {
      context_p->token.extra_value = LEXER_NUMBER_BINARY;
      source_p += 2;

      if (source_p >= source_end_p
          || !lit_char_is_binary_digit (source_p[0]))
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_BIN_DIGIT);
      }

      do
      {
        source_p++;
        if (source_p < source_end_p && source_p[0] == LIT_CHAR_UNDERSCORE)
        {
          source_p++;
          if (source_p == source_end_p
              || source_p[0] > LIT_CHAR_9
              || source_p[0] < LIT_CHAR_0)
          {
            parser_raise_error (context_p, PARSER_ERR_INVALID_UNDERSCORE_IN_NUMBER);
          }
        }
      }
      while (source_p < source_end_p
               && lit_char_is_binary_digit (source_p[0]));
    }
#endif /* JERRY_ESNEXT */
    else
    {
      can_be_float = true;
      source_p++;
    }
  }
  else
  {
    lexer_check_numbers (context_p, &source_p, source_end_p, LIT_CHAR_9, false);
    can_be_float = true;
  }

  if (can_be_float)
  {
    if (source_p < source_end_p
        && source_p[0] == LIT_CHAR_DOT)
    {
      source_p++;
#if JERRY_BUILTIN_BIGINT
      can_be_bigint = false;
#endif /* JERRY_BUILTIN_BIGINT */

#if JERRY_ESNEXT
      if (source_p < source_end_p && source_p[0] == LIT_CHAR_UNDERSCORE)
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_UNDERSCORE_IN_NUMBER);
      }
#endif /* JERRY_ESNEXT */
      lexer_check_numbers (context_p, &source_p, source_end_p, LIT_CHAR_9, false);
    }

    if (source_p < source_end_p
        && LEXER_TO_ASCII_LOWERCASE (source_p[0]) == LIT_CHAR_LOWERCASE_E)
    {
      source_p++;
#if JERRY_BUILTIN_BIGINT
      can_be_bigint = false;
#endif /* JERRY_BUILTIN_BIGINT */

      if (source_p < source_end_p
          && (source_p[0] == LIT_CHAR_PLUS || source_p[0] == LIT_CHAR_MINUS))
      {
        source_p++;
      }

      if (source_p >= source_end_p
          || source_p[0] < LIT_CHAR_0
          || source_p[0] > LIT_CHAR_9)
      {
        parser_raise_error (context_p, PARSER_ERR_MISSING_EXPONENT);
      }

      lexer_check_numbers (context_p, &source_p, source_end_p, LIT_CHAR_9, false);
    }
  }

#if JERRY_BUILTIN_BIGINT
  if (source_p < source_end_p && source_p[0] == LIT_CHAR_LOWERCASE_N)
  {
    if (!can_be_bigint)
    {
      parser_raise_error (context_p, PARSER_ERR_INVALID_BIGINT);
    }
    context_p->token.extra_value = LEXER_NUMBER_BIGINT;
    source_p++;
  }
#endif /* JERRY_BUILTIN_BIGINT */

  length = (size_t) (source_p - context_p->source_p);
  if (length > PARSER_MAXIMUM_STRING_LENGTH)
  {
    parser_raise_error (context_p, PARSER_ERR_NUMBER_TOO_LONG);
  }

  context_p->token.lit_location.length = (prop_length_t) length;
  PARSER_PLUS_EQUAL_LC (context_p->column, length);
  context_p->source_p = source_p;

  if (source_p < source_end_p && lexer_parse_identifier (context_p, LEXER_PARSE_CHECK_START_AND_RETURN))
  {
    parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_AFTER_NUMBER);
  }
} /* lexer_parse_number */