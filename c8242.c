psutil_proc_open_files(PyObject *self, PyObject *args) {
    long pid;
    int i;
    int cnt;
    int regular;
    int fd;
    char *path;
    struct kinfo_file *freep = NULL;
    struct kinfo_file *kif;
    kinfo_proc kipp;
    PyObject *py_tuple = NULL;
    PyObject *py_path = NULL;
    PyObject *py_retlist = PyList_New(0);

    if (py_retlist == NULL)
        return NULL;
    if (! PyArg_ParseTuple(args, "l", &pid))
        goto error;
    if (psutil_kinfo_proc(pid, &kipp) == -1)
        goto error;

    errno = 0;
    freep = kinfo_getfile(pid, &cnt);
    if (freep == NULL) {
        psutil_raise_for_pid(pid, "kinfo_getfile()");
        goto error;
    }

    for (i = 0; i < cnt; i++) {
        kif = &freep[i];

#ifdef PSUTIL_FREEBSD
        regular = (kif->kf_type == KF_TYPE_VNODE) && \
            (kif->kf_vnode_type == KF_VTYPE_VREG);
        fd = kif->kf_fd;
        path = kif->kf_path;
#elif PSUTIL_OPENBSD
        regular = (kif->f_type == DTYPE_VNODE) && (kif->v_type == VREG);
        fd = kif->fd_fd;
        // XXX - it appears path is not exposed in the kinfo_file struct.
        path = "";
#elif PSUTIL_NETBSD
        regular = (kif->ki_ftype == DTYPE_VNODE) && (kif->ki_vtype == VREG);
        fd = kif->ki_fd;
        // XXX - it appears path is not exposed in the kinfo_file struct.
        path = "";
#endif
        if (regular == 1) {
            py_path = PyUnicode_DecodeFSDefault(path);
            if (! py_path)
                goto error;
            py_tuple = Py_BuildValue("(Oi)", py_path, fd);
            if (py_tuple == NULL)
                goto error;
            if (PyList_Append(py_retlist, py_tuple))
                goto error;
            Py_DECREF(py_path);
            Py_DECREF(py_tuple);
        }
    }
    free(freep);
    return py_retlist;

error:
    Py_XDECREF(py_tuple);
    Py_DECREF(py_retlist);
    if (freep != NULL)
        free(freep);
    return NULL;
}