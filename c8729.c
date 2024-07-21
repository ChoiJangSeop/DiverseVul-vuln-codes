arg(identifier arg, expr_ty annotation, int lineno, int col_offset, int
    end_lineno, int end_col_offset, PyArena *arena)
{
    arg_ty p;
    if (!arg) {
        PyErr_SetString(PyExc_ValueError,
                        "field arg is required for arg");
        return NULL;
    }
    p = (arg_ty)PyArena_Malloc(arena, sizeof(*p));
    if (!p)
        return NULL;
    p->arg = arg;
    p->annotation = annotation;
    p->lineno = lineno;
    p->col_offset = col_offset;
    p->end_lineno = end_lineno;
    p->end_col_offset = end_col_offset;
    return p;
}