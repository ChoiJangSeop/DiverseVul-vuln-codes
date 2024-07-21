obj2ast_stmt(PyObject* obj, stmt_ty* out, PyArena* arena)
{
    int isinstance;

    PyObject *tmp = NULL;
    int lineno;
    int col_offset;
    int end_lineno;
    int end_col_offset;

    if (obj == Py_None) {
        *out = NULL;
        return 0;
    }
    if (_PyObject_LookupAttrId(obj, &PyId_lineno, &tmp) < 0) {
        return 1;
    }
    if (tmp == NULL) {
        PyErr_SetString(PyExc_TypeError, "required field \"lineno\" missing from stmt");
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
        PyErr_SetString(PyExc_TypeError, "required field \"col_offset\" missing from stmt");
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
    isinstance = PyObject_IsInstance(obj, (PyObject*)FunctionDef_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        identifier name;
        arguments_ty args;
        asdl_seq* body;
        asdl_seq* decorator_list;
        expr_ty returns;

        if (_PyObject_LookupAttrId(obj, &PyId_name, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"name\" missing from FunctionDef");
            return 1;
        }
        else {
            int res;
            res = obj2ast_identifier(tmp, &name, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_args, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"args\" missing from FunctionDef");
            return 1;
        }
        else {
            int res;
            res = obj2ast_arguments(tmp, &args, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_body, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"body\" missing from FunctionDef");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "FunctionDef field \"body\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            body = _Py_asdl_seq_new(len, arena);
            if (body == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "FunctionDef field \"body\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(body, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_decorator_list, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"decorator_list\" missing from FunctionDef");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "FunctionDef field \"decorator_list\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            decorator_list = _Py_asdl_seq_new(len, arena);
            if (decorator_list == NULL) goto failed;
            for (i = 0; i < len; i++) {
                expr_ty val;
                res = obj2ast_expr(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "FunctionDef field \"decorator_list\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(decorator_list, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_returns, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL || tmp == Py_None) {
            Py_CLEAR(tmp);
            returns = NULL;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &returns, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        *out = FunctionDef(name, args, body, decorator_list, returns, lineno,
                           col_offset, end_lineno, end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)AsyncFunctionDef_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        identifier name;
        arguments_ty args;
        asdl_seq* body;
        asdl_seq* decorator_list;
        expr_ty returns;

        if (_PyObject_LookupAttrId(obj, &PyId_name, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"name\" missing from AsyncFunctionDef");
            return 1;
        }
        else {
            int res;
            res = obj2ast_identifier(tmp, &name, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_args, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"args\" missing from AsyncFunctionDef");
            return 1;
        }
        else {
            int res;
            res = obj2ast_arguments(tmp, &args, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_body, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"body\" missing from AsyncFunctionDef");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "AsyncFunctionDef field \"body\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            body = _Py_asdl_seq_new(len, arena);
            if (body == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "AsyncFunctionDef field \"body\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(body, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_decorator_list, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"decorator_list\" missing from AsyncFunctionDef");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "AsyncFunctionDef field \"decorator_list\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            decorator_list = _Py_asdl_seq_new(len, arena);
            if (decorator_list == NULL) goto failed;
            for (i = 0; i < len; i++) {
                expr_ty val;
                res = obj2ast_expr(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "AsyncFunctionDef field \"decorator_list\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(decorator_list, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_returns, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL || tmp == Py_None) {
            Py_CLEAR(tmp);
            returns = NULL;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &returns, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        *out = AsyncFunctionDef(name, args, body, decorator_list, returns,
                                lineno, col_offset, end_lineno, end_col_offset,
                                arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)ClassDef_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        identifier name;
        asdl_seq* bases;
        asdl_seq* keywords;
        asdl_seq* body;
        asdl_seq* decorator_list;

        if (_PyObject_LookupAttrId(obj, &PyId_name, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"name\" missing from ClassDef");
            return 1;
        }
        else {
            int res;
            res = obj2ast_identifier(tmp, &name, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_bases, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"bases\" missing from ClassDef");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "ClassDef field \"bases\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            bases = _Py_asdl_seq_new(len, arena);
            if (bases == NULL) goto failed;
            for (i = 0; i < len; i++) {
                expr_ty val;
                res = obj2ast_expr(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "ClassDef field \"bases\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(bases, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_keywords, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"keywords\" missing from ClassDef");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "ClassDef field \"keywords\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            keywords = _Py_asdl_seq_new(len, arena);
            if (keywords == NULL) goto failed;
            for (i = 0; i < len; i++) {
                keyword_ty val;
                res = obj2ast_keyword(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "ClassDef field \"keywords\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(keywords, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_body, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"body\" missing from ClassDef");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "ClassDef field \"body\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            body = _Py_asdl_seq_new(len, arena);
            if (body == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "ClassDef field \"body\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(body, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_decorator_list, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"decorator_list\" missing from ClassDef");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "ClassDef field \"decorator_list\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            decorator_list = _Py_asdl_seq_new(len, arena);
            if (decorator_list == NULL) goto failed;
            for (i = 0; i < len; i++) {
                expr_ty val;
                res = obj2ast_expr(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "ClassDef field \"decorator_list\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(decorator_list, i, val);
            }
            Py_CLEAR(tmp);
        }
        *out = ClassDef(name, bases, keywords, body, decorator_list, lineno,
                        col_offset, end_lineno, end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)Return_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        expr_ty value;

        if (_PyObject_LookupAttrId(obj, &PyId_value, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL || tmp == Py_None) {
            Py_CLEAR(tmp);
            value = NULL;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &value, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        *out = Return(value, lineno, col_offset, end_lineno, end_col_offset,
                      arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)Delete_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        asdl_seq* targets;

        if (_PyObject_LookupAttrId(obj, &PyId_targets, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"targets\" missing from Delete");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "Delete field \"targets\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            targets = _Py_asdl_seq_new(len, arena);
            if (targets == NULL) goto failed;
            for (i = 0; i < len; i++) {
                expr_ty val;
                res = obj2ast_expr(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "Delete field \"targets\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(targets, i, val);
            }
            Py_CLEAR(tmp);
        }
        *out = Delete(targets, lineno, col_offset, end_lineno, end_col_offset,
                      arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)Assign_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        asdl_seq* targets;
        expr_ty value;

        if (_PyObject_LookupAttrId(obj, &PyId_targets, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"targets\" missing from Assign");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "Assign field \"targets\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            targets = _Py_asdl_seq_new(len, arena);
            if (targets == NULL) goto failed;
            for (i = 0; i < len; i++) {
                expr_ty val;
                res = obj2ast_expr(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "Assign field \"targets\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(targets, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_value, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"value\" missing from Assign");
            return 1;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &value, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        *out = Assign(targets, value, lineno, col_offset, end_lineno,
                      end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)AugAssign_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        expr_ty target;
        operator_ty op;
        expr_ty value;

        if (_PyObject_LookupAttrId(obj, &PyId_target, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"target\" missing from AugAssign");
            return 1;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &target, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_op, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"op\" missing from AugAssign");
            return 1;
        }
        else {
            int res;
            res = obj2ast_operator(tmp, &op, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_value, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"value\" missing from AugAssign");
            return 1;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &value, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        *out = AugAssign(target, op, value, lineno, col_offset, end_lineno,
                         end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)AnnAssign_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        expr_ty target;
        expr_ty annotation;
        expr_ty value;
        int simple;

        if (_PyObject_LookupAttrId(obj, &PyId_target, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"target\" missing from AnnAssign");
            return 1;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &target, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_annotation, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"annotation\" missing from AnnAssign");
            return 1;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &annotation, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_value, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL || tmp == Py_None) {
            Py_CLEAR(tmp);
            value = NULL;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &value, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_simple, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"simple\" missing from AnnAssign");
            return 1;
        }
        else {
            int res;
            res = obj2ast_int(tmp, &simple, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        *out = AnnAssign(target, annotation, value, simple, lineno, col_offset,
                         end_lineno, end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)For_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        expr_ty target;
        expr_ty iter;
        asdl_seq* body;
        asdl_seq* orelse;

        if (_PyObject_LookupAttrId(obj, &PyId_target, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"target\" missing from For");
            return 1;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &target, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_iter, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"iter\" missing from For");
            return 1;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &iter, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_body, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"body\" missing from For");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "For field \"body\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            body = _Py_asdl_seq_new(len, arena);
            if (body == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "For field \"body\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(body, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_orelse, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"orelse\" missing from For");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "For field \"orelse\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            orelse = _Py_asdl_seq_new(len, arena);
            if (orelse == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "For field \"orelse\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(orelse, i, val);
            }
            Py_CLEAR(tmp);
        }
        *out = For(target, iter, body, orelse, lineno, col_offset, end_lineno,
                   end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)AsyncFor_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        expr_ty target;
        expr_ty iter;
        asdl_seq* body;
        asdl_seq* orelse;

        if (_PyObject_LookupAttrId(obj, &PyId_target, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"target\" missing from AsyncFor");
            return 1;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &target, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_iter, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"iter\" missing from AsyncFor");
            return 1;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &iter, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_body, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"body\" missing from AsyncFor");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "AsyncFor field \"body\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            body = _Py_asdl_seq_new(len, arena);
            if (body == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "AsyncFor field \"body\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(body, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_orelse, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"orelse\" missing from AsyncFor");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "AsyncFor field \"orelse\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            orelse = _Py_asdl_seq_new(len, arena);
            if (orelse == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "AsyncFor field \"orelse\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(orelse, i, val);
            }
            Py_CLEAR(tmp);
        }
        *out = AsyncFor(target, iter, body, orelse, lineno, col_offset,
                        end_lineno, end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)While_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        expr_ty test;
        asdl_seq* body;
        asdl_seq* orelse;

        if (_PyObject_LookupAttrId(obj, &PyId_test, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"test\" missing from While");
            return 1;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &test, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_body, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"body\" missing from While");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "While field \"body\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            body = _Py_asdl_seq_new(len, arena);
            if (body == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "While field \"body\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(body, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_orelse, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"orelse\" missing from While");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "While field \"orelse\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            orelse = _Py_asdl_seq_new(len, arena);
            if (orelse == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "While field \"orelse\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(orelse, i, val);
            }
            Py_CLEAR(tmp);
        }
        *out = While(test, body, orelse, lineno, col_offset, end_lineno,
                     end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)If_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        expr_ty test;
        asdl_seq* body;
        asdl_seq* orelse;

        if (_PyObject_LookupAttrId(obj, &PyId_test, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"test\" missing from If");
            return 1;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &test, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_body, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"body\" missing from If");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "If field \"body\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            body = _Py_asdl_seq_new(len, arena);
            if (body == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "If field \"body\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(body, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_orelse, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"orelse\" missing from If");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "If field \"orelse\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            orelse = _Py_asdl_seq_new(len, arena);
            if (orelse == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "If field \"orelse\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(orelse, i, val);
            }
            Py_CLEAR(tmp);
        }
        *out = If(test, body, orelse, lineno, col_offset, end_lineno,
                  end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)With_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        asdl_seq* items;
        asdl_seq* body;

        if (_PyObject_LookupAttrId(obj, &PyId_items, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"items\" missing from With");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "With field \"items\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            items = _Py_asdl_seq_new(len, arena);
            if (items == NULL) goto failed;
            for (i = 0; i < len; i++) {
                withitem_ty val;
                res = obj2ast_withitem(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "With field \"items\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(items, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_body, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"body\" missing from With");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "With field \"body\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            body = _Py_asdl_seq_new(len, arena);
            if (body == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "With field \"body\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(body, i, val);
            }
            Py_CLEAR(tmp);
        }
        *out = With(items, body, lineno, col_offset, end_lineno,
                    end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)AsyncWith_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        asdl_seq* items;
        asdl_seq* body;

        if (_PyObject_LookupAttrId(obj, &PyId_items, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"items\" missing from AsyncWith");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "AsyncWith field \"items\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            items = _Py_asdl_seq_new(len, arena);
            if (items == NULL) goto failed;
            for (i = 0; i < len; i++) {
                withitem_ty val;
                res = obj2ast_withitem(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "AsyncWith field \"items\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(items, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_body, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"body\" missing from AsyncWith");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "AsyncWith field \"body\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            body = _Py_asdl_seq_new(len, arena);
            if (body == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "AsyncWith field \"body\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(body, i, val);
            }
            Py_CLEAR(tmp);
        }
        *out = AsyncWith(items, body, lineno, col_offset, end_lineno,
                         end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)Raise_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        expr_ty exc;
        expr_ty cause;

        if (_PyObject_LookupAttrId(obj, &PyId_exc, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL || tmp == Py_None) {
            Py_CLEAR(tmp);
            exc = NULL;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &exc, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_cause, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL || tmp == Py_None) {
            Py_CLEAR(tmp);
            cause = NULL;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &cause, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        *out = Raise(exc, cause, lineno, col_offset, end_lineno,
                     end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)Try_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        asdl_seq* body;
        asdl_seq* handlers;
        asdl_seq* orelse;
        asdl_seq* finalbody;

        if (_PyObject_LookupAttrId(obj, &PyId_body, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"body\" missing from Try");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "Try field \"body\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            body = _Py_asdl_seq_new(len, arena);
            if (body == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "Try field \"body\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(body, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_handlers, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"handlers\" missing from Try");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "Try field \"handlers\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            handlers = _Py_asdl_seq_new(len, arena);
            if (handlers == NULL) goto failed;
            for (i = 0; i < len; i++) {
                excepthandler_ty val;
                res = obj2ast_excepthandler(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "Try field \"handlers\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(handlers, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_orelse, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"orelse\" missing from Try");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "Try field \"orelse\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            orelse = _Py_asdl_seq_new(len, arena);
            if (orelse == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "Try field \"orelse\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(orelse, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_finalbody, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"finalbody\" missing from Try");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "Try field \"finalbody\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            finalbody = _Py_asdl_seq_new(len, arena);
            if (finalbody == NULL) goto failed;
            for (i = 0; i < len; i++) {
                stmt_ty val;
                res = obj2ast_stmt(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "Try field \"finalbody\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(finalbody, i, val);
            }
            Py_CLEAR(tmp);
        }
        *out = Try(body, handlers, orelse, finalbody, lineno, col_offset,
                   end_lineno, end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)Assert_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        expr_ty test;
        expr_ty msg;

        if (_PyObject_LookupAttrId(obj, &PyId_test, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"test\" missing from Assert");
            return 1;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &test, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_msg, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL || tmp == Py_None) {
            Py_CLEAR(tmp);
            msg = NULL;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &msg, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        *out = Assert(test, msg, lineno, col_offset, end_lineno,
                      end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)Import_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        asdl_seq* names;

        if (_PyObject_LookupAttrId(obj, &PyId_names, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"names\" missing from Import");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "Import field \"names\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            names = _Py_asdl_seq_new(len, arena);
            if (names == NULL) goto failed;
            for (i = 0; i < len; i++) {
                alias_ty val;
                res = obj2ast_alias(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "Import field \"names\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(names, i, val);
            }
            Py_CLEAR(tmp);
        }
        *out = Import(names, lineno, col_offset, end_lineno, end_col_offset,
                      arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)ImportFrom_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        identifier module;
        asdl_seq* names;
        int level;

        if (_PyObject_LookupAttrId(obj, &PyId_module, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL || tmp == Py_None) {
            Py_CLEAR(tmp);
            module = NULL;
        }
        else {
            int res;
            res = obj2ast_identifier(tmp, &module, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_names, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"names\" missing from ImportFrom");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "ImportFrom field \"names\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            names = _Py_asdl_seq_new(len, arena);
            if (names == NULL) goto failed;
            for (i = 0; i < len; i++) {
                alias_ty val;
                res = obj2ast_alias(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "ImportFrom field \"names\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(names, i, val);
            }
            Py_CLEAR(tmp);
        }
        if (_PyObject_LookupAttrId(obj, &PyId_level, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL || tmp == Py_None) {
            Py_CLEAR(tmp);
            level = 0;
        }
        else {
            int res;
            res = obj2ast_int(tmp, &level, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        *out = ImportFrom(module, names, level, lineno, col_offset, end_lineno,
                          end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)Global_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        asdl_seq* names;

        if (_PyObject_LookupAttrId(obj, &PyId_names, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"names\" missing from Global");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "Global field \"names\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            names = _Py_asdl_seq_new(len, arena);
            if (names == NULL) goto failed;
            for (i = 0; i < len; i++) {
                identifier val;
                res = obj2ast_identifier(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "Global field \"names\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(names, i, val);
            }
            Py_CLEAR(tmp);
        }
        *out = Global(names, lineno, col_offset, end_lineno, end_col_offset,
                      arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)Nonlocal_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        asdl_seq* names;

        if (_PyObject_LookupAttrId(obj, &PyId_names, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"names\" missing from Nonlocal");
            return 1;
        }
        else {
            int res;
            Py_ssize_t len;
            Py_ssize_t i;
            if (!PyList_Check(tmp)) {
                PyErr_Format(PyExc_TypeError, "Nonlocal field \"names\" must be a list, not a %.200s", tmp->ob_type->tp_name);
                goto failed;
            }
            len = PyList_GET_SIZE(tmp);
            names = _Py_asdl_seq_new(len, arena);
            if (names == NULL) goto failed;
            for (i = 0; i < len; i++) {
                identifier val;
                res = obj2ast_identifier(PyList_GET_ITEM(tmp, i), &val, arena);
                if (res != 0) goto failed;
                if (len != PyList_GET_SIZE(tmp)) {
                    PyErr_SetString(PyExc_RuntimeError, "Nonlocal field \"names\" changed size during iteration");
                    goto failed;
                }
                asdl_seq_SET(names, i, val);
            }
            Py_CLEAR(tmp);
        }
        *out = Nonlocal(names, lineno, col_offset, end_lineno, end_col_offset,
                        arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)Expr_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {
        expr_ty value;

        if (_PyObject_LookupAttrId(obj, &PyId_value, &tmp) < 0) {
            return 1;
        }
        if (tmp == NULL) {
            PyErr_SetString(PyExc_TypeError, "required field \"value\" missing from Expr");
            return 1;
        }
        else {
            int res;
            res = obj2ast_expr(tmp, &value, arena);
            if (res != 0) goto failed;
            Py_CLEAR(tmp);
        }
        *out = Expr(value, lineno, col_offset, end_lineno, end_col_offset,
                    arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)Pass_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {

        *out = Pass(lineno, col_offset, end_lineno, end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)Break_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {

        *out = Break(lineno, col_offset, end_lineno, end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }
    isinstance = PyObject_IsInstance(obj, (PyObject*)Continue_type);
    if (isinstance == -1) {
        return 1;
    }
    if (isinstance) {

        *out = Continue(lineno, col_offset, end_lineno, end_col_offset, arena);
        if (*out == NULL) goto failed;
        return 0;
    }

    PyErr_Format(PyExc_TypeError, "expected some sort of stmt, but got %R", obj);
    failed:
    Py_XDECREF(tmp);
    return 1;
}