parser_compiled_code_set_function_name (parser_context_t *context_p, /**< context */
                                        ecma_compiled_code_t *bytecode_p, /**< function literal index */
                                        uint16_t name_index, /**< function name literal index */
                                        uint32_t status_flags) /**< status flags */
{
  ecma_value_t *func_name_start_p;
  func_name_start_p = ecma_compiled_code_resolve_function_name ((const ecma_compiled_code_t *) bytecode_p);

  if (JERRY_UNLIKELY (!ecma_is_value_magic_string (*func_name_start_p, LIT_MAGIC_STRING__EMPTY)))
  {
    return;
  }

  parser_scope_stack_t *scope_stack_start_p = context_p->scope_stack_p;
  parser_scope_stack_t *scope_stack_p = scope_stack_start_p + context_p->scope_stack_top;

  while (scope_stack_p > scope_stack_start_p)
  {
    scope_stack_p--;

    if (scope_stack_p->map_from != PARSER_SCOPE_STACK_FUNC
        && scanner_decode_map_to (scope_stack_p) == name_index)
    {
      name_index = scope_stack_p->map_from;
      break;
    }
  }

  lexer_literal_t *name_lit_p = (lexer_literal_t *) PARSER_GET_LITERAL (name_index);

  if (name_lit_p->type != LEXER_IDENT_LITERAL && name_lit_p->type != LEXER_STRING_LITERAL)
  {
    return;
  }

  uint8_t *name_buffer_p = (uint8_t *) name_lit_p->u.char_p;
  uint32_t name_length = name_lit_p->prop.length;

  if (status_flags & (PARSER_IS_PROPERTY_GETTER | PARSER_IS_PROPERTY_SETTER))
  {
    name_length += 4;
    name_buffer_p = (uint8_t *) parser_malloc (context_p, name_length * sizeof (uint8_t));
    char *prefix_p = (status_flags & PARSER_IS_PROPERTY_GETTER) ? "get " : "set ";
    memcpy (name_buffer_p, prefix_p, 4);
    memcpy (name_buffer_p + 4, name_lit_p->u.char_p, name_lit_p->prop.length);
  }

  *func_name_start_p = ecma_find_or_create_literal_string (name_buffer_p, name_length);

  if (name_buffer_p != name_lit_p->u.char_p)
  {
    parser_free (name_buffer_p, name_length);
  }
} /* parser_compiled_code_set_function_name */