PyParser_AddToken(parser_state *ps, int type, char *str,
                  int lineno, int col_offset,
                  int end_lineno, int end_col_offset,
                  int *expected_ret)
{
    int ilabel;
    int err;

    D(printf("Token %s/'%s' ... ", _PyParser_TokenNames[type], str));

    /* Find out which label this token is */
    ilabel = classify(ps, type, str);
    if (ilabel < 0)
        return E_SYNTAX;

    /* Loop until the token is shifted or an error occurred */
    for (;;) {
        /* Fetch the current dfa and state */
        dfa *d = ps->p_stack.s_top->s_dfa;
        state *s = &d->d_state[ps->p_stack.s_top->s_state];

        D(printf(" DFA '%s', state %d:",
            d->d_name, ps->p_stack.s_top->s_state));

        /* Check accelerator */
        if (s->s_lower <= ilabel && ilabel < s->s_upper) {
            int x = s->s_accel[ilabel - s->s_lower];
            if (x != -1) {
                if (x & (1<<7)) {
                    /* Push non-terminal */
                    int nt = (x >> 8) + NT_OFFSET;
                    int arrow = x & ((1<<7)-1);
                    dfa *d1 = PyGrammar_FindDFA(
                        ps->p_grammar, nt);
                    if ((err = push(&ps->p_stack, nt, d1,
                        arrow, lineno, col_offset,
                        end_lineno, end_col_offset)) > 0) {
                        D(printf(" MemError: push\n"));
                        return err;
                    }
                    D(printf(" Push ...\n"));
                    continue;
                }

                /* Shift the token */
                if ((err = shift(&ps->p_stack, type, str,
                                x, lineno, col_offset,
                                end_lineno, end_col_offset)) > 0) {
                    D(printf(" MemError: shift.\n"));
                    return err;
                }
                D(printf(" Shift.\n"));
                /* Pop while we are in an accept-only state */
                while (s = &d->d_state
                                [ps->p_stack.s_top->s_state],
                    s->s_accept && s->s_narcs == 1) {
                    D(printf("  DFA '%s', state %d: "
                             "Direct pop.\n",
                             d->d_name,
                             ps->p_stack.s_top->s_state));
#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
#if 0
                    if (d->d_name[0] == 'i' &&
                        strcmp(d->d_name,
                           "import_stmt") == 0)
                        future_hack(ps);
#endif
#endif
                    s_pop(&ps->p_stack);
                    if (s_empty(&ps->p_stack)) {
                        D(printf("  ACCEPT.\n"));
                        return E_DONE;
                    }
                    d = ps->p_stack.s_top->s_dfa;
                }
                return E_OK;
            }
        }

        if (s->s_accept) {
#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
#if 0
            if (d->d_name[0] == 'i' &&
                strcmp(d->d_name, "import_stmt") == 0)
                future_hack(ps);
#endif
#endif
            /* Pop this dfa and try again */
            s_pop(&ps->p_stack);
            D(printf(" Pop ...\n"));
            if (s_empty(&ps->p_stack)) {
                D(printf(" Error: bottom of stack.\n"));
                return E_SYNTAX;
            }
            continue;
        }

        /* Stuck, report syntax error */
        D(printf(" Error.\n"));
        if (expected_ret) {
            if (s->s_lower == s->s_upper - 1) {
                /* Only one possible expected token */
                *expected_ret = ps->p_grammar->
                    g_ll.ll_label[s->s_lower].lb_type;
            }
            else
                *expected_ret = -1;
        }
        return E_SYNTAX;
    }
}