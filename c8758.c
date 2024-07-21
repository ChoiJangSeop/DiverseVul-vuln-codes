AsyncFor(expr_ty target, expr_ty iter, asdl_seq * body, asdl_seq * orelse, int
         lineno, int col_offset, int end_lineno, int end_col_offset, PyArena
         *arena)
{
    stmt_ty p;
    if (!target) {
        PyErr_SetString(PyExc_ValueError,
                        "field target is required for AsyncFor");
        return NULL;
    }
    if (!iter) {
        PyErr_SetString(PyExc_ValueError,
                        "field iter is required for AsyncFor");
        return NULL;
    }
    p = (stmt_ty)PyArena_Malloc(arena, sizeof(*p));
    if (!p)
        return NULL;
    p->kind = AsyncFor_kind;
    p->v.AsyncFor.target = target;
    p->v.AsyncFor.iter = iter;
    p->v.AsyncFor.body = body;
    p->v.AsyncFor.orelse = orelse;
    p->lineno = lineno;
    p->col_offset = col_offset;
    p->end_lineno = end_lineno;
    p->end_col_offset = end_col_offset;
    return p;
}