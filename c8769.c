ast_for_funcdef_impl(struct compiling *c, const node *n0,
                     asdl_seq *decorator_seq, bool is_async)
{
    /* funcdef: 'def' NAME parameters ['->' test] ':' suite */
    const node * const n = is_async ? CHILD(n0, 1) : n0;
    identifier name;
    arguments_ty args;
    asdl_seq *body;
    expr_ty returns = NULL;
    int name_i = 1;
    int end_lineno, end_col_offset;

    REQ(n, funcdef);

    name = NEW_IDENTIFIER(CHILD(n, name_i));
    if (!name)
        return NULL;
    if (forbidden_name(c, name, CHILD(n, name_i), 0))
        return NULL;
    args = ast_for_arguments(c, CHILD(n, name_i + 1));
    if (!args)
        return NULL;
    if (TYPE(CHILD(n, name_i+2)) == RARROW) {
        returns = ast_for_expr(c, CHILD(n, name_i + 3));
        if (!returns)
            return NULL;
        name_i += 2;
    }
    body = ast_for_suite(c, CHILD(n, name_i + 3));
    if (!body)
        return NULL;
    get_last_end_pos(body, &end_lineno, &end_col_offset);

    if (is_async)
        return AsyncFunctionDef(name, args, body, decorator_seq, returns,
                                LINENO(n0), n0->n_col_offset, end_lineno, end_col_offset, c->c_arena);
    else
        return FunctionDef(name, args, body, decorator_seq, returns,
                           LINENO(n), n->n_col_offset, end_lineno, end_col_offset, c->c_arena);
}