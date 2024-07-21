PyParser_ParseStringObject(const char *s, PyObject *filename,
                           grammar *g, int start,
                           perrdetail *err_ret, int *flags)
{
    struct tok_state *tok;
    int exec_input = start == file_input;

    if (initerr(err_ret, filename) < 0)
        return NULL;

    if (*flags & PyPARSE_IGNORE_COOKIE)
        tok = PyTokenizer_FromUTF8(s, exec_input);
    else
        tok = PyTokenizer_FromString(s, exec_input);
    if (tok == NULL) {
        err_ret->error = PyErr_Occurred() ? E_DECODE : E_NOMEM;
        return NULL;
    }

#ifndef PGEN
    Py_INCREF(err_ret->filename);
    tok->filename = err_ret->filename;
#endif
    return parsetok(tok, g, start, err_ret, flags);
}