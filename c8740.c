validate_node(node *tree)
{
    int type = TYPE(tree);
    int nch = NCH(tree);
    dfa *nt_dfa;
    state *dfa_state;
    int pos, arc;

    assert(ISNONTERMINAL(type));
    type -= NT_OFFSET;
    if (type >= _PyParser_Grammar.g_ndfas) {
        PyErr_Format(parser_error, "Unrecognized node type %d.", TYPE(tree));
        return 0;
    }
    nt_dfa = &_PyParser_Grammar.g_dfa[type];
    REQ(tree, nt_dfa->d_type);

    /* Run the DFA for this nonterminal. */
    dfa_state = &nt_dfa->d_state[nt_dfa->d_initial];
    for (pos = 0; pos < nch; ++pos) {
        node *ch = CHILD(tree, pos);
        int ch_type = TYPE(ch);
        for (arc = 0; arc < dfa_state->s_narcs; ++arc) {
            short a_label = dfa_state->s_arc[arc].a_lbl;
            assert(a_label < _PyParser_Grammar.g_ll.ll_nlabels);
            if (_PyParser_Grammar.g_ll.ll_label[a_label].lb_type == ch_type) {
                /* The child is acceptable; if non-terminal, validate it recursively. */
                if (ISNONTERMINAL(ch_type) && !validate_node(ch))
                    return 0;

                /* Update the state, and move on to the next child. */
                dfa_state = &nt_dfa->d_state[dfa_state->s_arc[arc].a_arrow];
                goto arc_found;
            }
        }
        /* What would this state have accepted? */
        {
            short a_label = dfa_state->s_arc->a_lbl;
            int next_type;
            if (!a_label) /* Wouldn't accept any more children */
                goto illegal_num_children;

            next_type = _PyParser_Grammar.g_ll.ll_label[a_label].lb_type;
            if (ISNONTERMINAL(next_type))
                PyErr_Format(parser_error, "Expected node type %d, got %d.",
                             next_type, ch_type);
            else
                PyErr_Format(parser_error, "Illegal terminal: expected %s.",
                             _PyParser_TokenNames[next_type]);
            return 0;
        }

arc_found:
        continue;
    }
    /* Are we in a final state? If so, return 1 for successful validation. */
    for (arc = 0; arc < dfa_state->s_narcs; ++arc) {
        if (!dfa_state->s_arc[arc].a_lbl) {
            return 1;
        }
    }

illegal_num_children:
    PyErr_Format(parser_error,
                 "Illegal number of children for %s node.", nt_dfa->d_name);
    return 0;
}