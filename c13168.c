lexer_compare_identifiers (parser_context_t *context_p, /**< context */
                           const lexer_lit_location_t *left_p, /**< left literal */
                           const lexer_lit_location_t *right_p) /**< right literal */
{
  prop_length_t length = left_p->length;

  if (length != right_p->length)
  {
    return false;
  }

  if (!left_p->has_escape)
  {
    return lexer_compare_identifier_to_chars (right_p->char_p, left_p->char_p, length);
  }

  if (!right_p->has_escape)
  {
    return lexer_compare_identifier_to_chars (left_p->char_p, right_p->char_p, length);
  }

  if (length <= 64)
  {
    uint8_t buf_p[64];
    lexer_convert_ident_to_cesu8 (buf_p, left_p->char_p, length);
    return lexer_compare_identifier_to_chars (right_p->char_p, buf_p, length);
  }

  uint8_t *dynamic_buf_p = parser_malloc (context_p, length);

  lexer_convert_ident_to_cesu8 (dynamic_buf_p, left_p->char_p, length);
  bool result = lexer_compare_identifier_to_chars (right_p->char_p, dynamic_buf_p, length);
  parser_free (dynamic_buf_p, length);

  return result;
} /* lexer_compare_identifiers */