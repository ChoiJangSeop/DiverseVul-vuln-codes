With(asdl_seq * items, asdl_seq * body, int lineno, int col_offset, int
     end_lineno, int end_col_offset, PyArena *arena)
{
    stmt_ty p;
    p = (stmt_ty)PyArena_Malloc(arena, sizeof(*p));
    if (!p)
        return NULL;
    p->kind = With_kind;
    p->v.With.items = items;
    p->v.With.body = body;
    p->lineno = lineno;
    p->col_offset = col_offset;
    p->end_lineno = end_lineno;
    p->end_col_offset = end_col_offset;
    return p;
}