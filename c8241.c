psutil_proc_environ(PyObject *self, PyObject *args) {
    int pid;
    PyObject *py_retdict = PyDict_New();
    PyObject *py_key = NULL;
    PyObject *py_val = NULL;
    struct procsinfo procbuf;
    long env_max;
    char *envbuf = NULL;
    char *curvar = NULL;
    char *separator = NULL;
    int ret;

    if (py_retdict == NULL)
        return NULL;
    if (!PyArg_ParseTuple(args, "i", &pid))
        goto error;
    env_max = sysconf(_SC_ARG_MAX);
    envbuf = malloc(env_max);
    if (envbuf == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    procbuf.pi_pid = pid;
    ret = getevars(&procbuf, sizeof(procbuf), envbuf, ARG_MAX);
    if (ret == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    curvar = envbuf;
    /* getevars will always append an extra NULL to end the arg list,
     * even if the buffer is not big enough (even though it is supposed
     * to be) so the following 'while' is safe */
    while (*curvar != '\0') {
        separator = strchr(curvar, '=');
        if (separator != NULL) {
            py_key = PyUnicode_DecodeFSDefaultAndSize(
                curvar,
                (Py_ssize_t)(separator - curvar)
            );
            if (!py_key)
                goto error;
            py_val = PyUnicode_DecodeFSDefault(separator + 1);
            if (!py_val)
                goto error;
            if (PyDict_SetItem(py_retdict, py_key, py_val))
                goto error;
            Py_DECREF(py_key);
            Py_DECREF(py_val);
        }
        curvar = strchr(curvar, '\0') + 1;
    }

    free(envbuf);

    return py_retdict;

error:
    if (envbuf != NULL)
        free(envbuf);
    Py_XDECREF(py_retdict);
    Py_XDECREF(py_key);
    Py_XDECREF(py_val);
    return NULL;
}