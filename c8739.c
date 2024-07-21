PyInit__ast(void)
{
    PyObject *m, *d;
    if (!init_types()) return NULL;
    m = PyModule_Create(&_astmodule);
    if (!m) return NULL;
    d = PyModule_GetDict(m);
    if (PyDict_SetItemString(d, "AST", (PyObject*)&AST_type) < 0) return NULL;
    if (PyModule_AddIntMacro(m, PyCF_ONLY_AST) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "mod", (PyObject*)mod_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "Module", (PyObject*)Module_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Interactive", (PyObject*)Interactive_type) <
        0) return NULL;
    if (PyDict_SetItemString(d, "Expression", (PyObject*)Expression_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "Suite", (PyObject*)Suite_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "stmt", (PyObject*)stmt_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "FunctionDef", (PyObject*)FunctionDef_type) <
        0) return NULL;
    if (PyDict_SetItemString(d, "AsyncFunctionDef",
        (PyObject*)AsyncFunctionDef_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "ClassDef", (PyObject*)ClassDef_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "Return", (PyObject*)Return_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Delete", (PyObject*)Delete_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Assign", (PyObject*)Assign_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "AugAssign", (PyObject*)AugAssign_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "AnnAssign", (PyObject*)AnnAssign_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "For", (PyObject*)For_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "AsyncFor", (PyObject*)AsyncFor_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "While", (PyObject*)While_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "If", (PyObject*)If_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "With", (PyObject*)With_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "AsyncWith", (PyObject*)AsyncWith_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "Raise", (PyObject*)Raise_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Try", (PyObject*)Try_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "Assert", (PyObject*)Assert_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Import", (PyObject*)Import_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "ImportFrom", (PyObject*)ImportFrom_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "Global", (PyObject*)Global_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Nonlocal", (PyObject*)Nonlocal_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "Expr", (PyObject*)Expr_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "Pass", (PyObject*)Pass_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "Break", (PyObject*)Break_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Continue", (PyObject*)Continue_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "expr", (PyObject*)expr_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "BoolOp", (PyObject*)BoolOp_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "NamedExpr", (PyObject*)NamedExpr_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "BinOp", (PyObject*)BinOp_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "UnaryOp", (PyObject*)UnaryOp_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Lambda", (PyObject*)Lambda_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "IfExp", (PyObject*)IfExp_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Dict", (PyObject*)Dict_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "Set", (PyObject*)Set_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "ListComp", (PyObject*)ListComp_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "SetComp", (PyObject*)SetComp_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "DictComp", (PyObject*)DictComp_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "GeneratorExp", (PyObject*)GeneratorExp_type) <
        0) return NULL;
    if (PyDict_SetItemString(d, "Await", (PyObject*)Await_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Yield", (PyObject*)Yield_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "YieldFrom", (PyObject*)YieldFrom_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "Compare", (PyObject*)Compare_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Call", (PyObject*)Call_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "FormattedValue",
        (PyObject*)FormattedValue_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "JoinedStr", (PyObject*)JoinedStr_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "Constant", (PyObject*)Constant_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "Attribute", (PyObject*)Attribute_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "Subscript", (PyObject*)Subscript_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "Starred", (PyObject*)Starred_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Name", (PyObject*)Name_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "List", (PyObject*)List_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "Tuple", (PyObject*)Tuple_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "expr_context", (PyObject*)expr_context_type) <
        0) return NULL;
    if (PyDict_SetItemString(d, "Load", (PyObject*)Load_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "Store", (PyObject*)Store_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Del", (PyObject*)Del_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "AugLoad", (PyObject*)AugLoad_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "AugStore", (PyObject*)AugStore_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "Param", (PyObject*)Param_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "NamedStore", (PyObject*)NamedStore_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "slice", (PyObject*)slice_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Slice", (PyObject*)Slice_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "ExtSlice", (PyObject*)ExtSlice_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "Index", (PyObject*)Index_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "boolop", (PyObject*)boolop_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "And", (PyObject*)And_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "Or", (PyObject*)Or_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "operator", (PyObject*)operator_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "Add", (PyObject*)Add_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "Sub", (PyObject*)Sub_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "Mult", (PyObject*)Mult_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "MatMult", (PyObject*)MatMult_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Div", (PyObject*)Div_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "Mod", (PyObject*)Mod_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "Pow", (PyObject*)Pow_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "LShift", (PyObject*)LShift_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "RShift", (PyObject*)RShift_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "BitOr", (PyObject*)BitOr_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "BitXor", (PyObject*)BitXor_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "BitAnd", (PyObject*)BitAnd_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "FloorDiv", (PyObject*)FloorDiv_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "unaryop", (PyObject*)unaryop_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Invert", (PyObject*)Invert_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Not", (PyObject*)Not_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "UAdd", (PyObject*)UAdd_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "USub", (PyObject*)USub_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "cmpop", (PyObject*)cmpop_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Eq", (PyObject*)Eq_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "NotEq", (PyObject*)NotEq_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "Lt", (PyObject*)Lt_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "LtE", (PyObject*)LtE_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "Gt", (PyObject*)Gt_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "GtE", (PyObject*)GtE_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "Is", (PyObject*)Is_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "IsNot", (PyObject*)IsNot_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "In", (PyObject*)In_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "NotIn", (PyObject*)NotIn_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "comprehension", (PyObject*)comprehension_type)
        < 0) return NULL;
    if (PyDict_SetItemString(d, "excepthandler", (PyObject*)excepthandler_type)
        < 0) return NULL;
    if (PyDict_SetItemString(d, "ExceptHandler", (PyObject*)ExceptHandler_type)
        < 0) return NULL;
    if (PyDict_SetItemString(d, "arguments", (PyObject*)arguments_type) < 0)
        return NULL;
    if (PyDict_SetItemString(d, "arg", (PyObject*)arg_type) < 0) return NULL;
    if (PyDict_SetItemString(d, "keyword", (PyObject*)keyword_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "alias", (PyObject*)alias_type) < 0) return
        NULL;
    if (PyDict_SetItemString(d, "withitem", (PyObject*)withitem_type) < 0)
        return NULL;
    return m;
}