psutil_proc_memory_maps(PyObject *self, PyObject *args) {
    int pid;
    int fd = -1;
    char path[1000];
    char perms[10];
    const char *name;
    struct stat st;
    pstatus_t status;

    prxmap_t *xmap = NULL, *p;
    off_t size;
    size_t nread;
    int nmap;
    uintptr_t pr_addr_sz;
    uintptr_t stk_base_sz, brk_base_sz;
    const char *procfs_path;

    PyObject *py_tuple = NULL;
    PyObject *py_path = NULL;
    PyObject *py_retlist = PyList_New(0);

    if (py_retlist == NULL)
        return NULL;
    if (! PyArg_ParseTuple(args, "is", &pid, &procfs_path))
        goto error;

    sprintf(path, "%s/%i/status", procfs_path, pid);
    if (! psutil_file_to_struct(path, (void *)&status, sizeof(status)))
        goto error;

    sprintf(path, "%s/%i/xmap", procfs_path, pid);
    if (stat(path, &st) == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    size = st.st_size;

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    xmap = (prxmap_t *)malloc(size);
    if (xmap == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    nread = pread(fd, xmap, size, 0);
    nmap = nread / sizeof(prxmap_t);
    p = xmap;

    while (nmap) {
        nmap -= 1;
        if (p == NULL) {
            p += 1;
            continue;
        }

        perms[0] = '\0';
        pr_addr_sz = p->pr_vaddr + p->pr_size;

        // perms
        sprintf(perms, "%c%c%c%c", p->pr_mflags & MA_READ ? 'r' : '-',
                p->pr_mflags & MA_WRITE ? 'w' : '-',
                p->pr_mflags & MA_EXEC ? 'x' : '-',
                p->pr_mflags & MA_SHARED ? 's' : '-');

        // name
        if (strlen(p->pr_mapname) > 0) {
            name = p->pr_mapname;
        }
        else {
            if ((p->pr_mflags & MA_ISM) || (p->pr_mflags & MA_SHM)) {
                name = "[shmid]";
            }
            else {
                stk_base_sz = status.pr_stkbase + status.pr_stksize;
                brk_base_sz = status.pr_brkbase + status.pr_brksize;

                if ((pr_addr_sz > status.pr_stkbase) &&
                        (p->pr_vaddr < stk_base_sz)) {
                    name = "[stack]";
                }
                else if ((p->pr_mflags & MA_ANON) && \
                         (pr_addr_sz > status.pr_brkbase) && \
                         (p->pr_vaddr < brk_base_sz)) {
                    name = "[heap]";
                }
                else {
                    name = "[anon]";
                }
            }
        }

        py_path = PyUnicode_DecodeFSDefault(name);
        if (! py_path)
            goto error;
        py_tuple = Py_BuildValue(
            "kksOkkk",
            (unsigned long)p->pr_vaddr,
            (unsigned long)pr_addr_sz,
            perms,
            py_path,
            (unsigned long)p->pr_rss * p->pr_pagesize,
            (unsigned long)p->pr_anon * p->pr_pagesize,
            (unsigned long)p->pr_locked * p->pr_pagesize);
        if (!py_tuple)
            goto error;
        if (PyList_Append(py_retlist, py_tuple))
            goto error;
        Py_DECREF(py_path);
        Py_DECREF(py_tuple);

        // increment pointer
        p += 1;
    }

    close(fd);
    free(xmap);
    return py_retlist;

error:
    if (fd != -1)
        close(fd);
    Py_XDECREF(py_tuple);
    Py_XDECREF(py_path);
    Py_DECREF(py_retlist);
    if (xmap != NULL)
        free(xmap);
    return NULL;
}