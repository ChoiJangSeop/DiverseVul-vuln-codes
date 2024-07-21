psutil_users(PyObject *self, PyObject *args) {
    struct utmpx *utx;
    PyObject *py_username = NULL;
    PyObject *py_tty = NULL;
    PyObject *py_hostname = NULL;
    PyObject *py_tuple = NULL;
    PyObject *py_retlist = PyList_New(0);

    if (py_retlist == NULL)
        return NULL;
    while ((utx = getutxent()) != NULL) {
        if (utx->ut_type != USER_PROCESS)
            continue;
        py_username = PyUnicode_DecodeFSDefault(utx->ut_user);
        if (! py_username)
            goto error;
        py_tty = PyUnicode_DecodeFSDefault(utx->ut_line);
        if (! py_tty)
            goto error;
        py_hostname = PyUnicode_DecodeFSDefault(utx->ut_host);
        if (! py_hostname)
            goto error;
        py_tuple = Py_BuildValue(
            "(OOOfi)",
            py_username,              // username
            py_tty,                   // tty
            py_hostname,              // hostname
            (float)utx->ut_tv.tv_sec, // start time
            utx->ut_pid               // process id
        );
        if (!py_tuple) {
            endutxent();
            goto error;
        }
        if (PyList_Append(py_retlist, py_tuple)) {
            endutxent();
            goto error;
        }
        Py_DECREF(py_username);
        Py_DECREF(py_tty);
        Py_DECREF(py_hostname);
        Py_DECREF(py_tuple);
    }

    endutxent();
    return py_retlist;

error:
    Py_XDECREF(py_username);
    Py_XDECREF(py_tty);
    Py_XDECREF(py_hostname);
    Py_XDECREF(py_tuple);
    Py_DECREF(py_retlist);
    return NULL;
}