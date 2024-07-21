parser_tagged_template_literal_append_strings (parser_context_t *context_p, /**< parser context */
                                               ecma_object_t *template_obj_p, /**< template object */
                                               ecma_object_t *raw_strings_p, /**< raw strings object */
                                               uint32_t prop_idx) /**< property index to set the values */
{
  lexer_lit_location_t *lit_loc_p = &context_p->token.lit_location;

  if (lit_loc_p->length == 0 && !lit_loc_p->has_escape)
  {
    ecma_builtin_helper_def_prop_by_index (template_obj_p,
                                           prop_idx,
                                           ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY),
                                           ECMA_PROPERTY_FLAG_ENUMERABLE);

    ecma_builtin_helper_def_prop_by_index (raw_strings_p,
                                           prop_idx,
                                           ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY),
                                           ECMA_PROPERTY_FLAG_ENUMERABLE);
    return;
  }

  uint8_t local_byte_array[LEXER_MAX_LITERAL_LOCAL_BUFFER_SIZE];
  const uint8_t *source_p = lexer_convert_literal_to_chars (context_p,
                                                            &context_p->token.lit_location,
                                                            local_byte_array,
                                                            LEXER_STRING_NO_OPTS);

  ecma_string_t *raw_str_p;
  ecma_string_t *cooked_str_p = ecma_new_ecma_string_from_utf8 (source_p, lit_loc_p->length);
  parser_free_allocated_buffer (context_p);

  if (lit_loc_p->has_escape)
  {
    context_p->source_p = context_p->token.lit_location.char_p - 1;
    lexer_parse_string (context_p, LEXER_STRING_RAW);
    source_p = lexer_convert_literal_to_chars (context_p,
                                               &context_p->token.lit_location,
                                               local_byte_array,
                                               LEXER_STRING_RAW);

    raw_str_p = ecma_new_ecma_string_from_utf8 (source_p, lit_loc_p->length);
    parser_free_allocated_buffer (context_p);
  }
  else
  {
    ecma_ref_ecma_string (cooked_str_p);
    raw_str_p = cooked_str_p;
  }

  ecma_builtin_helper_def_prop_by_index (template_obj_p,
                                         prop_idx,
                                         ecma_make_string_value (cooked_str_p),
                                         ECMA_PROPERTY_FLAG_ENUMERABLE);

  ecma_builtin_helper_def_prop_by_index (raw_strings_p,
                                         prop_idx,
                                         ecma_make_string_value (raw_str_p),
                                         ECMA_PROPERTY_FLAG_ENUMERABLE);

  ecma_deref_ecma_string (cooked_str_p);
  ecma_deref_ecma_string (raw_str_p);
} /* parser_tagged_template_literal_append_strings */