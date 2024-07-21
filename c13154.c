parser_module_append_export_name (parser_context_t *context_p) /**< parser context */
{
  if (!(context_p->status_flags & PARSER_MODULE_STORE_IDENT))
  {
    return;
  }

  context_p->module_identifier_lit_p = context_p->lit_object.literal_p;

  ecma_string_t *name_p = ecma_new_ecma_string_from_utf8 (context_p->lit_object.literal_p->u.char_p,
                                                          context_p->lit_object.literal_p->prop.length);

  if (parser_module_check_duplicate_export (context_p, name_p))
  {
    ecma_deref_ecma_string (name_p);
    parser_raise_error (context_p, PARSER_ERR_DUPLICATED_EXPORT_IDENTIFIER);
  }

  parser_module_add_names_to_node (context_p,
                                   name_p,
                                   name_p);
  ecma_deref_ecma_string (name_p);
} /* parser_module_append_export_name */