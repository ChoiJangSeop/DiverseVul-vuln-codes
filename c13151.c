parser_module_handle_module_specifier (parser_context_t *context_p, /**< parser context */
                                       ecma_module_node_t **node_list_p) /**< target node list */
{
  if (context_p->token.type != LEXER_LITERAL
      || context_p->token.lit_location.type != LEXER_STRING_LITERAL
      || context_p->token.lit_location.length == 0)
  {
    parser_raise_error (context_p, PARSER_ERR_STRING_EXPECTED);
  }

  lexer_construct_literal_object (context_p, &context_p->token.lit_location, LEXER_STRING_LITERAL);

  lexer_literal_t *path_p = context_p->lit_object.literal_p;

  lexer_next_token (context_p);

  /* The lexer_next_token may throw an error, so the path is constructed after its call. */
  ecma_string_t *path_string_p = ecma_new_ecma_string_from_utf8 (path_p->u.char_p, path_p->prop.length);

  ecma_module_node_t *node_p = JERRY_CONTEXT (module_current_p)->imports_p;
  ecma_module_node_t *last_node_p = NULL;

  /* Check if we have an import node with the same module request. */

  while (node_p != NULL)
  {
    if (ecma_compare_ecma_strings (ecma_get_string_from_value (node_p->u.path_or_module), path_string_p))
    {
      ecma_deref_ecma_string (path_string_p);
      break;
    }

    last_node_p = node_p;
    node_p = node_p->next_p;
  }

  if (node_p == NULL)
  {
    node_p = (ecma_module_node_t *) jmem_heap_alloc_block_null_on_error (sizeof (ecma_module_node_t));

    if (node_p == NULL)
    {
      ecma_deref_ecma_string (path_string_p);
      parser_raise_error (context_p, PARSER_ERR_OUT_OF_MEMORY);
    }

    if (last_node_p == NULL)
    {
      JERRY_CONTEXT (module_current_p)->imports_p = node_p;
    }
    else
    {
      last_node_p->next_p = node_p;
    }

    node_p->next_p = NULL;
    node_p->module_names_p = NULL;
    node_p->u.path_or_module = ecma_make_string_value (path_string_p);
  }

  /* Append to imports. */
  if (node_list_p == NULL)
  {
    parser_module_append_names (context_p, &node_p->module_names_p);
    return;
  }

  ecma_value_t *module_object_p = &node_p->u.path_or_module;

  node_p = *node_list_p;
  last_node_p = NULL;

  while (node_p != NULL)
  {
    if (node_p->u.module_object_p == module_object_p)
    {
      parser_module_append_names (context_p, &node_p->module_names_p);
      return;
    }

    last_node_p = node_p;
    node_p = node_p->next_p;
  }

  node_p = (ecma_module_node_t *) parser_malloc (context_p, sizeof (ecma_module_node_t));

  if (last_node_p == NULL)
  {
    *node_list_p = node_p;
  }
  else
  {
    last_node_p->next_p = node_p;
  }

  node_p->next_p = NULL;
  node_p->module_names_p = context_p->module_names_p;
  node_p->u.module_object_p = module_object_p;

  context_p->module_names_p = NULL;
} /* parser_module_handle_module_specifier */