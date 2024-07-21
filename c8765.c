ast2obj_mod(void* _o)
{
    mod_ty o = (mod_ty)_o;
    PyObject *result = NULL, *value = NULL;
    if (!o) {
        Py_RETURN_NONE;
    }

    switch (o->kind) {
    case Module_kind:
        result = PyType_GenericNew(Module_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_list(o->v.Module.body, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_body, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case Interactive_kind:
        result = PyType_GenericNew(Interactive_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_list(o->v.Interactive.body, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_body, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case Expression_kind:
        result = PyType_GenericNew(Expression_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_expr(o->v.Expression.body);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_body, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case Suite_kind:
        result = PyType_GenericNew(Suite_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_list(o->v.Suite.body, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_body, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    }
    return result;
failed:
    Py_XDECREF(value);
    Py_XDECREF(result);
    return NULL;
}