psutil_users(PyObject *self, PyObject *args) {
    struct utmpx *ut;
    PyObject *py_retlist = PyList_New(0);
    PyObject *py_tuple = NULL;
    PyObject *py_username = NULL;
    PyObject *py_tty = NULL;
    PyObject *py_hostname = NULL;
    PyObject *py_user_proc = NULL;

    if (py_retlist == NULL)
        return NULL;

    setutxent();
    while (NULL != (ut = getutxent())) {
        if (ut->ut_type == USER_PROCESS)
            py_user_proc = Py_True;
        else
            py_user_proc = Py_False;
        py_username = PyUnicode_DecodeFSDefault(ut->ut_user);
        if (! py_username)
            goto error;
        py_tty = PyUnicode_DecodeFSDefault(ut->ut_line);
        if (! py_tty)
            goto error;
        py_hostname = PyUnicode_DecodeFSDefault(ut->ut_host);
        if (! py_hostname)
            goto error;
        py_tuple = Py_BuildValue(
            "(OOOfOi)",
            py_username,              // username
            py_tty,                   // tty
            py_hostname,              // hostname
            (float)ut->ut_tv.tv_sec,  // tstamp
            py_user_proc,             // (bool) user process
            ut->ut_pid                // process id
        );
        if (py_tuple == NULL)
            goto error;
        if (PyList_Append(py_retlist, py_tuple))
            goto error;
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
    if (ut != NULL)
        endutxent();
    return NULL;
}