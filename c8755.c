ast_for_atom(struct compiling *c, const node *n)
{
    /* atom: '(' [yield_expr|testlist_comp] ')' | '[' [testlist_comp] ']'
       | '{' [dictmaker|testlist_comp] '}' | NAME | NUMBER | STRING+
       | '...' | 'None' | 'True' | 'False'
    */
    node *ch = CHILD(n, 0);

    switch (TYPE(ch)) {
    case NAME: {
        PyObject *name;
        const char *s = STR(ch);
        size_t len = strlen(s);
        if (len >= 4 && len <= 5) {
            if (!strcmp(s, "None"))
                return Constant(Py_None, LINENO(n), n->n_col_offset,
                                n->n_end_lineno, n->n_end_col_offset, c->c_arena);
            if (!strcmp(s, "True"))
                return Constant(Py_True, LINENO(n), n->n_col_offset,
                                n->n_end_lineno, n->n_end_col_offset, c->c_arena);
            if (!strcmp(s, "False"))
                return Constant(Py_False, LINENO(n), n->n_col_offset,
                                n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        }
        name = new_identifier(s, c);
        if (!name)
            return NULL;
        /* All names start in Load context, but may later be changed. */
        return Name(name, Load, LINENO(n), n->n_col_offset,
                    n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    case STRING: {
        expr_ty str = parsestrplus(c, n);
        if (!str) {
            const char *errtype = NULL;
            if (PyErr_ExceptionMatches(PyExc_UnicodeError))
                errtype = "unicode error";
            else if (PyErr_ExceptionMatches(PyExc_ValueError))
                errtype = "value error";
            if (errtype) {
                PyObject *type, *value, *tback, *errstr;
                PyErr_Fetch(&type, &value, &tback);
                errstr = PyObject_Str(value);
                if (errstr) {
                    ast_error(c, n, "(%s) %U", errtype, errstr);
                    Py_DECREF(errstr);
                }
                else {
                    PyErr_Clear();
                    ast_error(c, n, "(%s) unknown error", errtype);
                }
                Py_DECREF(type);
                Py_XDECREF(value);
                Py_XDECREF(tback);
            }
            return NULL;
        }
        return str;
    }
    case NUMBER: {
        PyObject *pynum = parsenumber(c, STR(ch));
        if (!pynum)
            return NULL;

        if (PyArena_AddPyObject(c->c_arena, pynum) < 0) {
            Py_DECREF(pynum);
            return NULL;
        }
        return Constant(pynum, LINENO(n), n->n_col_offset,
                        n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    case ELLIPSIS: /* Ellipsis */
        return Constant(Py_Ellipsis, LINENO(n), n->n_col_offset,
                        n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    case LPAR: /* some parenthesized expressions */
        ch = CHILD(n, 1);

        if (TYPE(ch) == RPAR)
            return Tuple(NULL, Load, LINENO(n), n->n_col_offset,
                         n->n_end_lineno, n->n_end_col_offset, c->c_arena);

        if (TYPE(ch) == yield_expr)
            return ast_for_expr(c, ch);

        /* testlist_comp: test ( comp_for | (',' test)* [','] ) */
        if (NCH(ch) == 1) {
            return ast_for_testlist(c, ch);
        }

        if (TYPE(CHILD(ch, 1)) == comp_for) {
            return copy_location(ast_for_genexp(c, ch), n);
        }
        else {
            return copy_location(ast_for_testlist(c, ch), n);
        }
    case LSQB: /* list (or list comprehension) */
        ch = CHILD(n, 1);

        if (TYPE(ch) == RSQB)
            return List(NULL, Load, LINENO(n), n->n_col_offset,
                        n->n_end_lineno, n->n_end_col_offset, c->c_arena);

        REQ(ch, testlist_comp);
        if (NCH(ch) == 1 || TYPE(CHILD(ch, 1)) == COMMA) {
            asdl_seq *elts = seq_for_testlist(c, ch);
            if (!elts)
                return NULL;

            return List(elts, Load, LINENO(n), n->n_col_offset,
                        n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        }
        else {
            return copy_location(ast_for_listcomp(c, ch), n);
        }
    case LBRACE: {
        /* dictorsetmaker: ( ((test ':' test | '**' test)
         *                    (comp_for | (',' (test ':' test | '**' test))* [','])) |
         *                   ((test | '*' test)
         *                    (comp_for | (',' (test | '*' test))* [','])) ) */
        expr_ty res;
        ch = CHILD(n, 1);
        if (TYPE(ch) == RBRACE) {
            /* It's an empty dict. */
            return Dict(NULL, NULL, LINENO(n), n->n_col_offset,
                        n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        }
        else {
            int is_dict = (TYPE(CHILD(ch, 0)) == DOUBLESTAR);
            if (NCH(ch) == 1 ||
                    (NCH(ch) > 1 &&
                     TYPE(CHILD(ch, 1)) == COMMA)) {
                /* It's a set display. */
                res = ast_for_setdisplay(c, ch);
            }
            else if (NCH(ch) > 1 &&
                    TYPE(CHILD(ch, 1)) == comp_for) {
                /* It's a set comprehension. */
                res = ast_for_setcomp(c, ch);
            }
            else if (NCH(ch) > 3 - is_dict &&
                    TYPE(CHILD(ch, 3 - is_dict)) == comp_for) {
                /* It's a dictionary comprehension. */
                if (is_dict) {
                    ast_error(c, n, "dict unpacking cannot be used in "
                            "dict comprehension");
                    return NULL;
                }
                res = ast_for_dictcomp(c, ch);
            }
            else {
                /* It's a dictionary display. */
                res = ast_for_dictdisplay(c, ch);
            }
            return copy_location(res, n);
        }
    }
    default:
        PyErr_Format(PyExc_SystemError, "unhandled atom %d", TYPE(ch));
        return NULL;
    }
}