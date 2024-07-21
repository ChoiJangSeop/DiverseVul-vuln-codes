obj2ast_arg(PyObject* obj, arg_ty* out, PyArena* arena)
{
    PyObject* tmp = NULL;
    identifier arg;
    expr_ty annotation;
    int lineno;
    int col_offset;
    int end_lineno;
    int end_col_offset;

    if (_PyObject_LookupAttrId(obj, &PyId_arg, &tmp) < 0) {
        return 1;
    }
    if (tmp == NULL) {
        PyErr_SetString(PyExc_TypeError, "required field \"arg\" missing from arg");
        return 1;
    }
    else {
        int res;
        res = obj2ast_identifier(tmp, &arg, arena);
        if (res != 0) goto failed;
        Py_CLEAR(tmp);
    }
    if (_PyObject_LookupAttrId(obj, &PyId_annotation, &tmp) < 0) {
        return 1;
    }
    if (tmp == NULL || tmp == Py_None) {
        Py_CLEAR(tmp);
        annotation = NULL;
    }
    else {
        int res;
        res = obj2ast_expr(tmp, &annotation, arena);
        if (res != 0) goto failed;
        Py_CLEAR(tmp);
    }
    if (_PyObject_LookupAttrId(obj, &PyId_lineno, &tmp) < 0) {
        return 1;
    }
    if (tmp == NULL) {
        PyErr_SetString(PyExc_TypeError, "required field \"lineno\" missing from arg");
        return 1;
    }
    else {
        int res;
        res = obj2ast_int(tmp, &lineno, arena);
        if (res != 0) goto failed;
        Py_CLEAR(tmp);
    }
    if (_PyObject_LookupAttrId(obj, &PyId_col_offset, &tmp) < 0) {
        return 1;
    }
    if (tmp == NULL) {
        PyErr_SetString(PyExc_TypeError, "required field \"col_offset\" missing from arg");
        return 1;
    }
    else {
        int res;
        res = obj2ast_int(tmp, &col_offset, arena);
        if (res != 0) goto failed;
        Py_CLEAR(tmp);
    }
    if (_PyObject_LookupAttrId(obj, &PyId_end_lineno, &tmp) < 0) {
        return 1;
    }
    if (tmp == NULL || tmp == Py_None) {
        Py_CLEAR(tmp);
        end_lineno = 0;
    }
    else {
        int res;
        res = obj2ast_int(tmp, &end_lineno, arena);
        if (res != 0) goto failed;
        Py_CLEAR(tmp);
    }
    if (_PyObject_LookupAttrId(obj, &PyId_end_col_offset, &tmp) < 0) {
        return 1;
    }
    if (tmp == NULL || tmp == Py_None) {
        Py_CLEAR(tmp);
        end_col_offset = 0;
    }
    else {
        int res;
        res = obj2ast_int(tmp, &end_col_offset, arena);
        if (res != 0) goto failed;
        Py_CLEAR(tmp);
    }
    *out = arg(arg, annotation, lineno, col_offset, end_lineno, end_col_offset,
               arena);
    return 0;
failed:
    Py_XDECREF(tmp);
    return 1;
}