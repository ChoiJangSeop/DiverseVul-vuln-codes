ast2obj_arg(void* _o)
{
    arg_ty o = (arg_ty)_o;
    PyObject *result = NULL, *value = NULL;
    if (!o) {
        Py_RETURN_NONE;
    }

    result = PyType_GenericNew(arg_type, NULL, NULL);
    if (!result) return NULL;
    value = ast2obj_identifier(o->arg);
    if (!value) goto failed;
    if (_PyObject_SetAttrId(result, &PyId_arg, value) == -1)
        goto failed;
    Py_DECREF(value);
    value = ast2obj_expr(o->annotation);
    if (!value) goto failed;
    if (_PyObject_SetAttrId(result, &PyId_annotation, value) == -1)
        goto failed;
    Py_DECREF(value);
    value = ast2obj_int(o->lineno);
    if (!value) goto failed;
    if (_PyObject_SetAttrId(result, &PyId_lineno, value) < 0)
        goto failed;
    Py_DECREF(value);
    value = ast2obj_int(o->col_offset);
    if (!value) goto failed;
    if (_PyObject_SetAttrId(result, &PyId_col_offset, value) < 0)
        goto failed;
    Py_DECREF(value);
    value = ast2obj_int(o->end_lineno);
    if (!value) goto failed;
    if (_PyObject_SetAttrId(result, &PyId_end_lineno, value) < 0)
        goto failed;
    Py_DECREF(value);
    value = ast2obj_int(o->end_col_offset);
    if (!value) goto failed;
    if (_PyObject_SetAttrId(result, &PyId_end_col_offset, value) < 0)
        goto failed;
    Py_DECREF(value);
    return result;
failed:
    Py_XDECREF(value);
    Py_XDECREF(result);
    return NULL;
}