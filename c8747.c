parsetok(struct tok_state *tok, grammar *g, int start, perrdetail *err_ret,
         int *flags)
{
    parser_state *ps;
    node *n;
    int started = 0;
    int col_offset, end_col_offset;

    if ((ps = PyParser_New(g, start)) == NULL) {
        err_ret->error = E_NOMEM;
        PyTokenizer_Free(tok);
        return NULL;
    }
#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
    if (*flags & PyPARSE_BARRY_AS_BDFL)
        ps->p_flags |= CO_FUTURE_BARRY_AS_BDFL;
#endif

    for (;;) {
        char *a, *b;
        int type;
        size_t len;
        char *str;
        col_offset = -1;
        int lineno;
        const char *line_start;

        type = PyTokenizer_Get(tok, &a, &b);
        if (type == ERRORTOKEN) {
            err_ret->error = tok->done;
            break;
        }
        if (type == ENDMARKER && started) {
            type = NEWLINE; /* Add an extra newline */
            started = 0;
            /* Add the right number of dedent tokens,
               except if a certain flag is given --
               codeop.py uses this. */
            if (tok->indent &&
                !(*flags & PyPARSE_DONT_IMPLY_DEDENT))
            {
                tok->pendin = -tok->indent;
                tok->indent = 0;
            }
        }
        else
            started = 1;
        len = (a != NULL && b != NULL) ? b - a : 0;
        str = (char *) PyObject_MALLOC(len + 1);
        if (str == NULL) {
            err_ret->error = E_NOMEM;
            break;
        }
        if (len > 0)
            strncpy(str, a, len);
        str[len] = '\0';

#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
        if (type == NOTEQUAL) {
            if (!(ps->p_flags & CO_FUTURE_BARRY_AS_BDFL) &&
                            strcmp(str, "!=")) {
                PyObject_FREE(str);
                err_ret->error = E_SYNTAX;
                break;
            }
            else if ((ps->p_flags & CO_FUTURE_BARRY_AS_BDFL) &&
                            strcmp(str, "<>")) {
                PyObject_FREE(str);
                err_ret->expected = NOTEQUAL;
                err_ret->error = E_SYNTAX;
                break;
            }
        }
#endif

        /* Nodes of type STRING, especially multi line strings
           must be handled differently in order to get both
           the starting line number and the column offset right.
           (cf. issue 16806) */
        lineno = type == STRING ? tok->first_lineno : tok->lineno;
        line_start = type == STRING ? tok->multi_line_start : tok->line_start;
        if (a != NULL && a >= line_start) {
            col_offset = Py_SAFE_DOWNCAST(a - line_start,
                                          intptr_t, int);
        }
        else {
            col_offset = -1;
        }

        if (b != NULL && b >= tok->line_start) {
            end_col_offset = Py_SAFE_DOWNCAST(b - tok->line_start,
                                              intptr_t, int);
        }
        else {
            end_col_offset = -1;
        }
        if ((err_ret->error =
             PyParser_AddToken(ps, (int)type, str,
                               lineno, col_offset, tok->lineno, end_col_offset,
                               &(err_ret->expected))) != E_OK) {
            if (err_ret->error != E_DONE) {
                PyObject_FREE(str);
                err_ret->token = type;
            }
            break;
        }
    }

    if (err_ret->error == E_DONE) {
        n = ps->p_tree;
        ps->p_tree = NULL;

#ifndef PGEN
        /* Check that the source for a single input statement really
           is a single statement by looking at what is left in the
           buffer after parsing.  Trailing whitespace and comments
           are OK.  */
        if (start == single_input) {
            char *cur = tok->cur;
            char c = *tok->cur;

            for (;;) {
                while (c == ' ' || c == '\t' || c == '\n' || c == '\014')
                    c = *++cur;

                if (!c)
                    break;

                if (c != '#') {
                    err_ret->error = E_BADSINGLE;
                    PyNode_Free(n);
                    n = NULL;
                    break;
                }

                /* Suck up comment. */
                while (c && c != '\n')
                    c = *++cur;
            }
        }
#endif
    }
    else
        n = NULL;

#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
    *flags = ps->p_flags;
#endif
    PyParser_Delete(ps);

    if (n == NULL) {
        if (tok->done == E_EOF)
            err_ret->error = E_EOF;
        err_ret->lineno = tok->lineno;
        if (tok->buf != NULL) {
            size_t len;
            assert(tok->cur - tok->buf < INT_MAX);
            /* if we've managed to parse a token, point the offset to its start,
             * else use the current reading position of the tokenizer
             */
            err_ret->offset = col_offset != -1 ? col_offset + 1 : ((int)(tok->cur - tok->buf));
            len = tok->inp - tok->buf;
            err_ret->text = (char *) PyObject_MALLOC(len + 1);
            if (err_ret->text != NULL) {
                if (len > 0)
                    strncpy(err_ret->text, tok->buf, len);
                err_ret->text[len] = '\0';
            }
        }
    } else if (tok->encoding != NULL) {
        /* 'nodes->n_str' uses PyObject_*, while 'tok->encoding' was
         * allocated using PyMem_
         */
        node* r = PyNode_New(encoding_decl);
        if (r)
            r->n_str = PyObject_MALLOC(strlen(tok->encoding)+1);
        if (!r || !r->n_str) {
            err_ret->error = E_NOMEM;
            if (r)
                PyObject_FREE(r);
            n = NULL;
            goto done;
        }
        strcpy(r->n_str, tok->encoding);
        PyMem_FREE(tok->encoding);
        tok->encoding = NULL;
        r->n_nchildren = 1;
        r->n_child = n;
        n = r;
    }

done:
    PyTokenizer_Free(tok);

    if (n != NULL) {
        _PyNode_FinalizeEndPos(n);
    }
    return n;
}