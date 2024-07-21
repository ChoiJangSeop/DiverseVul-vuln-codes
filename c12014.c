check_and_convert_grammar (void)
{
  /* Grammar has been read.  Do some checking.  */
  if (nrules == 0)
    complain (NULL, fatal, _("no rules in the input grammar"));

  /* If the user did not define her EOFTOKEN, do it now. */
  if (!eoftoken)
    {
      eoftoken = symbol_get ("YYEOF", empty_loc);
      eoftoken->content->class = token_sym;
      eoftoken->content->number = 0;
      /* Value specified by POSIX.  */
      eoftoken->content->code = 0;
      {
        symbol *alias = symbol_get ("$end", empty_loc);
        symbol_class_set (alias, token_sym, empty_loc, false);
        symbol_make_alias (eoftoken, alias, empty_loc);
      }
    }

  /* Report any undefined symbols and consider them nonterminals.  */
  symbols_check_defined ();

  /* Find the start symbol if no %start.  */
  if (!start_flag)
    {
      symbol *start = find_start_symbol ();
      grammar_start_symbol_set (start, start->location);
    }

  /* Insert the initial rule, whose line is that of the first rule
     (not that of the start symbol):

     $accept: %start $end.  */
  {
    symbol_list *p = symbol_list_sym_new (acceptsymbol, empty_loc);
    p->rhs_loc = grammar->rhs_loc;
    p->next = symbol_list_sym_new (startsymbol, empty_loc);
    p->next->next = symbol_list_sym_new (eoftoken, empty_loc);
    p->next->next->next = symbol_list_sym_new (NULL, empty_loc);
    p->next->next->next->next = grammar;
    nrules += 1;
    nritems += 3;
    grammar = p;
  }

  aver (nsyms <= SYMBOL_NUMBER_MAXIMUM);
  aver (nsyms == ntokens + nnterms);

  /* Assign the symbols their symbol numbers.  */
  symbols_pack ();

  /* Scan rule actions after invoking symbol_check_alias_consistency
     (in symbols_pack above) so that token types are set correctly
     before the rule action type checking.

     Before invoking grammar_rule_check_and_complete (in packgram
     below) on any rule, make sure all actions have already been
     scanned in order to set 'used' flags.  Otherwise, checking that a
     midrule's $$ should be set will not always work properly because
     the check must forward-reference the midrule's parent rule.  For
     the same reason, all the 'used' flags must be set before checking
     whether to remove '$' from any midrule symbol name (also in
     packgram).  */
  for (symbol_list *sym = grammar; sym; sym = sym->next)
    code_props_translate_code (&sym->action_props);

  /* Convert the grammar into the format described in gram.h.  */
  packgram ();

  /* The grammar as a symbol_list is no longer needed. */
  symbol_list_free (grammar);
}