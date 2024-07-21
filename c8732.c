ast_for_arg(struct compiling *c, const node *n)
{
    identifier name;
    expr_ty annotation = NULL;
    node *ch;
    arg_ty ret;

    assert(TYPE(n) == tfpdef || TYPE(n) == vfpdef);
    ch = CHILD(n, 0);
    name = NEW_IDENTIFIER(ch);
    if (!name)
        return NULL;
    if (forbidden_name(c, name, ch, 0))
        return NULL;

    if (NCH(n) == 3 && TYPE(CHILD(n, 1)) == COLON) {
        annotation = ast_for_expr(c, CHILD(n, 2));
        if (!annotation)
            return NULL;
    }

    ret = arg(name, annotation, LINENO(n), n->n_col_offset,
              n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    if (!ret)
        return NULL;
    return ret;
}