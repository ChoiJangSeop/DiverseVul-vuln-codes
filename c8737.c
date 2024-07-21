ast2obj_stmt(void* _o)
{
    stmt_ty o = (stmt_ty)_o;
    PyObject *result = NULL, *value = NULL;
    if (!o) {
        Py_RETURN_NONE;
    }

    switch (o->kind) {
    case FunctionDef_kind:
        result = PyType_GenericNew(FunctionDef_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_identifier(o->v.FunctionDef.name);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_name, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_arguments(o->v.FunctionDef.args);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_args, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.FunctionDef.body, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_body, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.FunctionDef.decorator_list, ast2obj_expr);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_decorator_list, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_expr(o->v.FunctionDef.returns);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_returns, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case AsyncFunctionDef_kind:
        result = PyType_GenericNew(AsyncFunctionDef_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_identifier(o->v.AsyncFunctionDef.name);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_name, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_arguments(o->v.AsyncFunctionDef.args);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_args, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.AsyncFunctionDef.body, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_body, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.AsyncFunctionDef.decorator_list,
                             ast2obj_expr);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_decorator_list, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_expr(o->v.AsyncFunctionDef.returns);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_returns, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case ClassDef_kind:
        result = PyType_GenericNew(ClassDef_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_identifier(o->v.ClassDef.name);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_name, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.ClassDef.bases, ast2obj_expr);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_bases, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.ClassDef.keywords, ast2obj_keyword);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_keywords, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.ClassDef.body, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_body, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.ClassDef.decorator_list, ast2obj_expr);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_decorator_list, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case Return_kind:
        result = PyType_GenericNew(Return_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_expr(o->v.Return.value);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_value, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case Delete_kind:
        result = PyType_GenericNew(Delete_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_list(o->v.Delete.targets, ast2obj_expr);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_targets, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case Assign_kind:
        result = PyType_GenericNew(Assign_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_list(o->v.Assign.targets, ast2obj_expr);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_targets, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_expr(o->v.Assign.value);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_value, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case AugAssign_kind:
        result = PyType_GenericNew(AugAssign_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_expr(o->v.AugAssign.target);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_target, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_operator(o->v.AugAssign.op);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_op, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_expr(o->v.AugAssign.value);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_value, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case AnnAssign_kind:
        result = PyType_GenericNew(AnnAssign_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_expr(o->v.AnnAssign.target);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_target, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_expr(o->v.AnnAssign.annotation);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_annotation, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_expr(o->v.AnnAssign.value);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_value, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_int(o->v.AnnAssign.simple);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_simple, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case For_kind:
        result = PyType_GenericNew(For_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_expr(o->v.For.target);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_target, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_expr(o->v.For.iter);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_iter, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.For.body, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_body, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.For.orelse, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_orelse, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case AsyncFor_kind:
        result = PyType_GenericNew(AsyncFor_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_expr(o->v.AsyncFor.target);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_target, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_expr(o->v.AsyncFor.iter);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_iter, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.AsyncFor.body, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_body, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.AsyncFor.orelse, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_orelse, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case While_kind:
        result = PyType_GenericNew(While_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_expr(o->v.While.test);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_test, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.While.body, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_body, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.While.orelse, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_orelse, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case If_kind:
        result = PyType_GenericNew(If_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_expr(o->v.If.test);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_test, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.If.body, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_body, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.If.orelse, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_orelse, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case With_kind:
        result = PyType_GenericNew(With_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_list(o->v.With.items, ast2obj_withitem);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_items, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.With.body, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_body, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case AsyncWith_kind:
        result = PyType_GenericNew(AsyncWith_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_list(o->v.AsyncWith.items, ast2obj_withitem);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_items, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.AsyncWith.body, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_body, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case Raise_kind:
        result = PyType_GenericNew(Raise_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_expr(o->v.Raise.exc);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_exc, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_expr(o->v.Raise.cause);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_cause, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case Try_kind:
        result = PyType_GenericNew(Try_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_list(o->v.Try.body, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_body, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.Try.handlers, ast2obj_excepthandler);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_handlers, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.Try.orelse, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_orelse, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.Try.finalbody, ast2obj_stmt);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_finalbody, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case Assert_kind:
        result = PyType_GenericNew(Assert_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_expr(o->v.Assert.test);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_test, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_expr(o->v.Assert.msg);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_msg, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case Import_kind:
        result = PyType_GenericNew(Import_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_list(o->v.Import.names, ast2obj_alias);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_names, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case ImportFrom_kind:
        result = PyType_GenericNew(ImportFrom_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_identifier(o->v.ImportFrom.module);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_module, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_list(o->v.ImportFrom.names, ast2obj_alias);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_names, value) == -1)
            goto failed;
        Py_DECREF(value);
        value = ast2obj_int(o->v.ImportFrom.level);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_level, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case Global_kind:
        result = PyType_GenericNew(Global_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_list(o->v.Global.names, ast2obj_identifier);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_names, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case Nonlocal_kind:
        result = PyType_GenericNew(Nonlocal_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_list(o->v.Nonlocal.names, ast2obj_identifier);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_names, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case Expr_kind:
        result = PyType_GenericNew(Expr_type, NULL, NULL);
        if (!result) goto failed;
        value = ast2obj_expr(o->v.Expr.value);
        if (!value) goto failed;
        if (_PyObject_SetAttrId(result, &PyId_value, value) == -1)
            goto failed;
        Py_DECREF(value);
        break;
    case Pass_kind:
        result = PyType_GenericNew(Pass_type, NULL, NULL);
        if (!result) goto failed;
        break;
    case Break_kind:
        result = PyType_GenericNew(Break_type, NULL, NULL);
        if (!result) goto failed;
        break;
    case Continue_kind:
        result = PyType_GenericNew(Continue_type, NULL, NULL);
        if (!result) goto failed;
        break;
    }
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