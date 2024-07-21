scanner_scope_find_lexical_declaration (parser_context_t *context_p, /**< context */
                                        lexer_lit_location_t *literal_p) /**< literal */
{
  ecma_string_t *name_p;
  uint32_t flags = context_p->global_status_flags;

  if (!(flags & ECMA_PARSE_EVAL)
      || (!(flags & ECMA_PARSE_DIRECT_EVAL) && (context_p->status_flags & PARSER_IS_STRICT)))
  {
    return false;
  }

  if (JERRY_LIKELY (!literal_p->has_escape))
  {
    name_p = ecma_new_ecma_string_from_utf8 (literal_p->char_p, literal_p->length);
  }
  else
  {
    uint8_t *destination_p = (uint8_t *) scanner_malloc (context_p, literal_p->length);

    lexer_convert_ident_to_cesu8 (destination_p, literal_p->char_p, literal_p->length);

    name_p = ecma_new_ecma_string_from_utf8 (destination_p, literal_p->length);
    scanner_free (destination_p, literal_p->length);
  }

  ecma_object_t *lex_env_p;

  if (flags & ECMA_PARSE_DIRECT_EVAL)
  {
    lex_env_p = JERRY_CONTEXT (vm_top_context_p)->lex_env_p;

    while (lex_env_p->type_flags_refs & ECMA_OBJECT_FLAG_BLOCK)
    {
      if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
      {
        ecma_property_t *property_p = ecma_find_named_property (lex_env_p, name_p);

        if (property_p != NULL && ecma_is_property_enumerable (*property_p))
        {
          ecma_deref_ecma_string (name_p);
          return true;
        }
      }

      JERRY_ASSERT (lex_env_p->u2.outer_reference_cp != JMEM_CP_NULL);
      lex_env_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, lex_env_p->u2.outer_reference_cp);
    }
  }
  else
  {
    lex_env_p = ecma_get_global_scope (ecma_builtin_get_global ());
  }

  if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
  {
    ecma_property_t *property_p = ecma_find_named_property (lex_env_p, name_p);

    if (property_p != NULL && ecma_is_property_enumerable (*property_p))
    {
      ecma_deref_ecma_string (name_p);
      return true;
    }
  }

  ecma_deref_ecma_string (name_p);
  return false;
} /* scanner_scope_find_lexical_declaration */