mod_ty PyAST_obj2mod(PyObject* ast, PyArena* arena, int mode)
{
    mod_ty res;
    PyObject *req_type[3];
    char *req_name[] = {"Module", "Expression", "Interactive"};
    int isinstance;

    req_type[0] = (PyObject*)Module_type;
    req_type[1] = (PyObject*)Expression_type;
    req_type[2] = (PyObject*)Interactive_type;

    assert(0 <= mode && mode <= 2);

    if (!init_types())
        return NULL;

    isinstance = PyObject_IsInstance(ast, req_type[mode]);
    if (isinstance == -1)
        return NULL;
    if (!isinstance) {
        PyErr_Format(PyExc_TypeError, "expected %s node, got %.400s",
                     req_name[mode], Py_TYPE(ast)->tp_name);
        return NULL;
    }
    if (obj2ast_mod(ast, &res, arena) != 0)
        return NULL;
    else
        return res;
}