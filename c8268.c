psutil_users(PyObject *self, PyObject *args) {
    PyObject *py_retlist = PyList_New(0);
    PyObject *py_username = NULL;
    PyObject *py_tty = NULL;
    PyObject *py_hostname = NULL;
    PyObject *py_tuple = NULL;

    if (py_retlist == NULL)
        return NULL;

#if (defined(__FreeBSD_version) && (__FreeBSD_version < 900000)) || PSUTIL_OPENBSD
    struct utmp ut;
    FILE *fp;

    fp = fopen(_PATH_UTMP, "r");
    if (fp == NULL) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, _PATH_UTMP);
        goto error;
    }

    while (fread(&ut, sizeof(ut), 1, fp) == 1) {
        if (*ut.ut_name == '\0')
            continue;
        py_username = PyUnicode_DecodeFSDefault(ut.ut_name);
        if (! py_username)
            goto error;
        py_tty = PyUnicode_DecodeFSDefault(ut.ut_line);
        if (! py_tty)
            goto error;
        py_hostname = PyUnicode_DecodeFSDefault(ut.ut_host);
        if (! py_hostname)
            goto error;
        py_tuple = Py_BuildValue(
            "(OOOfi)",
            py_username,        // username
            py_tty,             // tty
            py_hostname,        // hostname
            (float)ut.ut_time,  // start time
#ifdef PSUTIL_OPENBSD
            -1                  // process id (set to None later)
#else
            ut.ut_pid           // process id
#endif
        );
        if (!py_tuple) {
            fclose(fp);
            goto error;
        }
        if (PyList_Append(py_retlist, py_tuple)) {
            fclose(fp);
            goto error;
        }
        Py_DECREF(py_username);
        Py_DECREF(py_tty);
        Py_DECREF(py_hostname);
        Py_DECREF(py_tuple);
    }

    fclose(fp);
#else
    struct utmpx *utx;
    setutxent();
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
            py_username,   // username
            py_tty,        // tty
            py_hostname,   // hostname
            (float)utx->ut_tv.tv_sec,  // start time
#ifdef PSUTIL_OPENBSD
            -1             // process id (set to None later)
#else
            utx->ut_pid    // process id
#endif
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
#endif
    return py_retlist;

error:
    Py_XDECREF(py_username);
    Py_XDECREF(py_tty);
    Py_XDECREF(py_hostname);
    Py_XDECREF(py_tuple);
    Py_DECREF(py_retlist);
    return NULL;
}