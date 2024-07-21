Assign(asdl_seq * targets, expr_ty value, int lineno, int col_offset, int
       end_lineno, int end_col_offset, PyArena *arena)
{
    stmt_ty p;
    if (!value) {
        PyErr_SetString(PyExc_ValueError,
                        "field value is required for Assign");
        return NULL;
    }
    p = (stmt_ty)PyArena_Malloc(arena, sizeof(*p));
    if (!p)
        return NULL;
    p->kind = Assign_kind;
    p->v.Assign.targets = targets;
    p->v.Assign.value = value;
    p->lineno = lineno;
    p->col_offset = col_offset;
    p->end_lineno = end_lineno;
    p->end_col_offset = end_col_offset;
    return p;
}