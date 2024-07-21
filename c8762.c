ast_for_import_stmt(struct compiling *c, const node *n)
{
    /*
      import_stmt: import_name | import_from
      import_name: 'import' dotted_as_names
      import_from: 'from' (('.' | '...')* dotted_name | ('.' | '...')+)
                   'import' ('*' | '(' import_as_names ')' | import_as_names)
    */
    int lineno;
    int col_offset;
    int i;
    asdl_seq *aliases;

    REQ(n, import_stmt);
    lineno = LINENO(n);
    col_offset = n->n_col_offset;
    n = CHILD(n, 0);
    if (TYPE(n) == import_name) {
        n = CHILD(n, 1);
        REQ(n, dotted_as_names);
        aliases = _Py_asdl_seq_new((NCH(n) + 1) / 2, c->c_arena);
        if (!aliases)
                return NULL;
        for (i = 0; i < NCH(n); i += 2) {
            alias_ty import_alias = alias_for_import_name(c, CHILD(n, i), 1);
            if (!import_alias)
                return NULL;
            asdl_seq_SET(aliases, i / 2, import_alias);
        }
        // Even though n is modified above, the end position is not changed
        return Import(aliases, lineno, col_offset,
                      n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    else if (TYPE(n) == import_from) {
        int n_children;
        int idx, ndots = 0;
        const node *n_copy = n;
        alias_ty mod = NULL;
        identifier modname = NULL;

       /* Count the number of dots (for relative imports) and check for the
          optional module name */
        for (idx = 1; idx < NCH(n); idx++) {
            if (TYPE(CHILD(n, idx)) == dotted_name) {
                mod = alias_for_import_name(c, CHILD(n, idx), 0);
                if (!mod)
                    return NULL;
                idx++;
                break;
            } else if (TYPE(CHILD(n, idx)) == ELLIPSIS) {
                /* three consecutive dots are tokenized as one ELLIPSIS */
                ndots += 3;
                continue;
            } else if (TYPE(CHILD(n, idx)) != DOT) {
                break;
            }
            ndots++;
        }
        idx++; /* skip over the 'import' keyword */
        switch (TYPE(CHILD(n, idx))) {
        case STAR:
            /* from ... import * */
            n = CHILD(n, idx);
            n_children = 1;
            break;
        case LPAR:
            /* from ... import (x, y, z) */
            n = CHILD(n, idx + 1);
            n_children = NCH(n);
            break;
        case import_as_names:
            /* from ... import x, y, z */
            n = CHILD(n, idx);
            n_children = NCH(n);
            if (n_children % 2 == 0) {
                ast_error(c, n, "trailing comma not allowed without"
                             " surrounding parentheses");
                return NULL;
            }
            break;
        default:
            ast_error(c, n, "Unexpected node-type in from-import");
            return NULL;
        }

        aliases = _Py_asdl_seq_new((n_children + 1) / 2, c->c_arena);
        if (!aliases)
            return NULL;

        /* handle "from ... import *" special b/c there's no children */
        if (TYPE(n) == STAR) {
            alias_ty import_alias = alias_for_import_name(c, n, 1);
            if (!import_alias)
                return NULL;
            asdl_seq_SET(aliases, 0, import_alias);
        }
        else {
            for (i = 0; i < NCH(n); i += 2) {
                alias_ty import_alias = alias_for_import_name(c, CHILD(n, i), 1);
                if (!import_alias)
                    return NULL;
                asdl_seq_SET(aliases, i / 2, import_alias);
            }
        }
        if (mod != NULL)
            modname = mod->name;
        return ImportFrom(modname, aliases, ndots, lineno, col_offset,
                          n_copy->n_end_lineno, n_copy->n_end_col_offset,
                          c->c_arena);
    }
    PyErr_Format(PyExc_SystemError,
                 "unknown import statement: starts with command '%s'",
                 STR(CHILD(n, 0)));
    return NULL;
}