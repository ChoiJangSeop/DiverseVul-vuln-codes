builtin_compile_impl(PyObject *module, PyObject *source, PyObject *filename,
                     const char *mode, int flags, int dont_inherit,
                     int optimize)
/*[clinic end generated code: output=1fa176e33452bb63 input=0ff726f595eb9fcd]*/
{
    PyObject *source_copy;
    const char *str;
    int compile_mode = -1;
    int is_ast;
    PyCompilerFlags cf;
    int start[] = {Py_file_input, Py_eval_input, Py_single_input};
    PyObject *result;

    cf.cf_flags = flags | PyCF_SOURCE_IS_UTF8;

    if (flags &
        ~(PyCF_MASK | PyCF_MASK_OBSOLETE | PyCF_DONT_IMPLY_DEDENT | PyCF_ONLY_AST))
    {
        PyErr_SetString(PyExc_ValueError,
                        "compile(): unrecognised flags");
        goto error;
    }
    /* XXX Warn if (supplied_flags & PyCF_MASK_OBSOLETE) != 0? */

    if (optimize < -1 || optimize > 2) {
        PyErr_SetString(PyExc_ValueError,
                        "compile(): invalid optimize value");
        goto error;
    }

    if (!dont_inherit) {
        PyEval_MergeCompilerFlags(&cf);
    }

    if (strcmp(mode, "exec") == 0)
        compile_mode = 0;
    else if (strcmp(mode, "eval") == 0)
        compile_mode = 1;
    else if (strcmp(mode, "single") == 0)
        compile_mode = 2;
    else {
        PyErr_SetString(PyExc_ValueError,
                        "compile() mode must be 'exec', 'eval' or 'single'");
        goto error;
    }

    is_ast = PyAST_Check(source);
    if (is_ast == -1)
        goto error;
    if (is_ast) {
        if (flags & PyCF_ONLY_AST) {
            Py_INCREF(source);
            result = source;
        }
        else {
            PyArena *arena;
            mod_ty mod;

            arena = PyArena_New();
            if (arena == NULL)
                goto error;
            mod = PyAST_obj2mod(source, arena, compile_mode);
            if (mod == NULL) {
                PyArena_Free(arena);
                goto error;
            }
            if (!PyAST_Validate(mod)) {
                PyArena_Free(arena);
                goto error;
            }
            result = (PyObject*)PyAST_CompileObject(mod, filename,
                                                    &cf, optimize, arena);
            PyArena_Free(arena);
        }
        goto finally;
    }

    str = source_as_string(source, "compile", "string, bytes or AST", &cf, &source_copy);
    if (str == NULL)
        goto error;

    result = Py_CompileStringObject(str, filename, start[compile_mode], &cf, optimize);
    Py_XDECREF(source_copy);
    goto finally;

error:
    result = NULL;
finally:
    Py_DECREF(filename);
    return result;
}