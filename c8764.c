For(expr_ty target, expr_ty iter, asdl_seq * body, asdl_seq * orelse, int
    lineno, int col_offset, int end_lineno, int end_col_offset, PyArena *arena)
{
    stmt_ty p;
    if (!target) {
        PyErr_SetString(PyExc_ValueError,
                        "field target is required for For");
        return NULL;
    }
    if (!iter) {
        PyErr_SetString(PyExc_ValueError,
                        "field iter is required for For");
        return NULL;
    }
    p = (stmt_ty)PyArena_Malloc(arena, sizeof(*p));
    if (!p)
        return NULL;
    p->kind = For_kind;
    p->v.For.target = target;
    p->v.For.iter = iter;
    p->v.For.body = body;
    p->v.For.orelse = orelse;
    p->lineno = lineno;
    p->col_offset = col_offset;
    p->end_lineno = end_lineno;
    p->end_col_offset = end_col_offset;
    return p;
}