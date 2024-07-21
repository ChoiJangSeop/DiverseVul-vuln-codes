psutil_proc_environ(PyObject *self, PyObject *args) {
    int pid;
    char path[1000];
    psinfo_t info;
    const char *procfs_path;
    char **env = NULL;
    ssize_t env_count = -1;
    char *dm;
    int i = 0;
    PyObject *py_envname = NULL;
    PyObject *py_envval = NULL;
    PyObject *py_retdict = PyDict_New();

    if (! py_retdict)
        return PyErr_NoMemory();

    if (! PyArg_ParseTuple(args, "is", &pid, &procfs_path))
        return NULL;

    sprintf(path, "%s/%i/psinfo", procfs_path, pid);
    if (! psutil_file_to_struct(path, (void *)&info, sizeof(info)))
        goto error;

    if (! info.pr_envp) {
        AccessDenied("");
        goto error;
    }

    env = psutil_read_raw_env(info, procfs_path, &env_count);
    if (! env && env_count != 0)
        goto error;

    for (i=0; i<env_count; i++) {
        if (! env[i])
            break;

        dm = strchr(env[i], '=');
        if (! dm)
            continue;

        *dm = '\0';

        py_envname = PyUnicode_DecodeFSDefault(env[i]);
        if (! py_envname)
            goto error;

        py_envval = PyUnicode_DecodeFSDefault(dm+1);
        if (! py_envname)
            goto error;

        if (PyDict_SetItem(py_retdict, py_envname, py_envval) < 0)
            goto error;

        Py_DECREF(py_envname);
        Py_DECREF(py_envval);
    }

    psutil_free_cstrings_array(env, env_count);
    return py_retdict;

 error:
    if (env && env_count >= 0)
        psutil_free_cstrings_array(env, env_count);

    Py_XDECREF(py_envname);
    Py_XDECREF(py_envval);
    Py_XDECREF(py_retdict);
    return NULL;
}