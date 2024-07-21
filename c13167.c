lexer_convert_literal_to_chars (parser_context_t *context_p, /**< context */
                                const lexer_lit_location_t *literal_p, /**< literal location */
                                uint8_t *local_byte_array_p, /**< local byte array to store chars */
                                lexer_string_options_t opts) /**< options */
{
  JERRY_ASSERT (context_p->u.allocated_buffer_p == NULL);

  if (!literal_p->has_escape)
  {
    return literal_p->char_p;
  }

  uint8_t *destination_start_p;
  if (literal_p->length > LEXER_MAX_LITERAL_LOCAL_BUFFER_SIZE)
  {
    context_p->u.allocated_buffer_p = (uint8_t *) parser_malloc_local (context_p, literal_p->length);
    context_p->allocated_buffer_size = literal_p->length;
    destination_start_p = context_p->u.allocated_buffer_p;
  }
  else
  {
    destination_start_p = local_byte_array_p;
  }

  if (literal_p->type == LEXER_IDENT_LITERAL)
  {
    lexer_convert_ident_to_cesu8 (destination_start_p, literal_p->char_p, literal_p->length);
    return destination_start_p;
  }

  const uint8_t *source_p = literal_p->char_p;
  uint8_t *destination_p = destination_start_p;

  uint8_t str_end_character = source_p[-1];

#if JERRY_ESNEXT
  if (str_end_character == LIT_CHAR_RIGHT_BRACE)
  {
    str_end_character = LIT_CHAR_GRAVE_ACCENT;
  }

  bool is_raw = (opts & LEXER_STRING_RAW) != 0;
#else /* !JERRY_ESNEXT */
  JERRY_UNUSED (opts);
  bool is_raw = false;
#endif /* JERRY_ESNEXT */

  while (true)
  {
    if (*source_p == str_end_character)
    {
      break;
    }

    if (*source_p == LIT_CHAR_BACKSLASH && !is_raw)
    {
      uint8_t conv_character;

      source_p++;
      JERRY_ASSERT (source_p < context_p->source_end_p);

      /* Newline is ignored. */
      if (*source_p == LIT_CHAR_CR)
      {
        source_p++;
        JERRY_ASSERT (source_p < context_p->source_end_p);

        if (*source_p == LIT_CHAR_LF)
        {
          source_p++;
        }
        continue;
      }
      else if (*source_p == LIT_CHAR_LF)
      {
        source_p++;
        continue;
      }
      else if (*source_p == LEXER_NEWLINE_LS_PS_BYTE_1 && LEXER_NEWLINE_LS_PS_BYTE_23 (source_p))
      {
        source_p += 3;
        continue;
      }

      if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_3)
      {
        lit_code_point_t octal_number = (uint32_t) (*source_p - LIT_CHAR_0);

        source_p++;
        JERRY_ASSERT (source_p < context_p->source_end_p);

        if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
        {
          octal_number = octal_number * 8 + (uint32_t) (*source_p - LIT_CHAR_0);
          source_p++;
          JERRY_ASSERT (source_p < context_p->source_end_p);

          if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
          {
            octal_number = octal_number * 8 + (uint32_t) (*source_p - LIT_CHAR_0);
            source_p++;
            JERRY_ASSERT (source_p < context_p->source_end_p);
          }
        }

        destination_p += lit_code_point_to_cesu8_bytes (destination_p, octal_number);
        continue;
      }

      if (*source_p >= LIT_CHAR_4 && *source_p <= LIT_CHAR_7)
      {
        uint32_t octal_number = (uint32_t) (*source_p - LIT_CHAR_0);

        source_p++;
        JERRY_ASSERT (source_p < context_p->source_end_p);

        if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
        {
          octal_number = octal_number * 8 + (uint32_t) (*source_p - LIT_CHAR_0);
          source_p++;
          JERRY_ASSERT (source_p < context_p->source_end_p);
        }

        *destination_p++ = (uint8_t) octal_number;
        continue;
      }

      if (*source_p == LIT_CHAR_LOWERCASE_X || *source_p == LIT_CHAR_LOWERCASE_U)
      {
        source_p++;
        destination_p += lit_code_point_to_cesu8_bytes (destination_p,
                                                        lexer_unchecked_hex_to_character (&source_p));
        continue;
      }

      conv_character = *source_p;
      switch (*source_p)
      {
        case LIT_CHAR_LOWERCASE_B:
        {
          conv_character = 0x08;
          break;
        }
        case LIT_CHAR_LOWERCASE_T:
        {
          conv_character = 0x09;
          break;
        }
        case LIT_CHAR_LOWERCASE_N:
        {
          conv_character = 0x0a;
          break;
        }
        case LIT_CHAR_LOWERCASE_V:
        {
          conv_character = 0x0b;
          break;
        }
        case LIT_CHAR_LOWERCASE_F:
        {
          conv_character = 0x0c;
          break;
        }
        case LIT_CHAR_LOWERCASE_R:
        {
          conv_character = 0x0d;
          break;
        }
      }

      if (conv_character != *source_p)
      {
        *destination_p++ = conv_character;
        source_p++;
        continue;
      }
    }
#if JERRY_ESNEXT
    else if (str_end_character == LIT_CHAR_GRAVE_ACCENT)
    {
      if (source_p[0] == LIT_CHAR_DOLLAR_SIGN
          && source_p[1] == LIT_CHAR_LEFT_BRACE)
      {
        source_p++;
        JERRY_ASSERT (source_p < context_p->source_end_p);
        break;
      }
      if (*source_p == LIT_CHAR_CR)
      {
        *destination_p++ = LIT_CHAR_LF;
        source_p++;
        if (*source_p != str_end_character
            && *source_p == LIT_CHAR_LF)
        {
          source_p++;
        }
        continue;
      }
      if ((*source_p == LIT_CHAR_BACKSLASH) && is_raw)
      {
        JERRY_ASSERT (source_p + 1 < context_p->source_end_p);
        if ((*(source_p + 1) == LIT_CHAR_GRAVE_ACCENT) || (*(source_p + 1) == LIT_CHAR_BACKSLASH))
        {
          *destination_p++ = *source_p++;
          *destination_p++ = *source_p++;
          continue;
        }
      }
    }
#endif /* JERRY_ESNEXT */

    if (*source_p >= LIT_UTF8_4_BYTE_MARKER)
    {
      /* Processing 4 byte unicode sequence (even if it is
        * after a backslash). Always converted to two 3 byte
        * long sequence. */
      lit_four_byte_utf8_char_to_cesu8 (destination_p, source_p);

      destination_p += 6;
      source_p += 4;
      continue;
    }

    *destination_p++ = *source_p++;

    /* There is no need to check the source_end_p
      * since the string is terminated by a quotation mark. */
    while (IS_UTF8_INTERMEDIATE_OCTET (*source_p))
    {
      *destination_p++ = *source_p++;
    }
  }

  JERRY_ASSERT (destination_p == destination_start_p + literal_p->length);

  return destination_start_p;
} /* lexer_convert_literal_to_chars */