ast_for_with_stmt(struct compiling *c, const node *n0, bool is_async)
{
    const node * const n = is_async ? CHILD(n0, 1) : n0;
    int i, n_items, end_lineno, end_col_offset;
    asdl_seq *items, *body;

    REQ(n, with_stmt);

    n_items = (NCH(n) - 2) / 2;
    items = _Py_asdl_seq_new(n_items, c->c_arena);
    if (!items)
        return NULL;
    for (i = 1; i < NCH(n) - 2; i += 2) {
        withitem_ty item = ast_for_with_item(c, CHILD(n, i));
        if (!item)
            return NULL;
        asdl_seq_SET(items, (i - 1) / 2, item);
    }

    body = ast_for_suite(c, CHILD(n, NCH(n) - 1));
    if (!body)
        return NULL;
    get_last_end_pos(body, &end_lineno, &end_col_offset);

    if (is_async)
        return AsyncWith(items, body, LINENO(n0), n0->n_col_offset,
                         end_lineno, end_col_offset, c->c_arena);
    else
        return With(items, body, LINENO(n), n->n_col_offset,
                    end_lineno, end_col_offset, c->c_arena);
}