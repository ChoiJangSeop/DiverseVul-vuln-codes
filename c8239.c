psutil_per_cpu_times(PyObject *self, PyObject *args) {
    natural_t cpu_count;
    natural_t i;
    processor_info_array_t info_array;
    mach_msg_type_number_t info_count;
    kern_return_t error;
    processor_cpu_load_info_data_t *cpu_load_info = NULL;
    int ret;
    PyObject *py_retlist = PyList_New(0);
    PyObject *py_cputime = NULL;

    if (py_retlist == NULL)
        return NULL;

    mach_port_t host_port = mach_host_self();
    error = host_processor_info(host_port, PROCESSOR_CPU_LOAD_INFO,
                                &cpu_count, &info_array, &info_count);
    if (error != KERN_SUCCESS) {
        PyErr_Format(
            PyExc_RuntimeError,
            "host_processor_info(PROCESSOR_CPU_LOAD_INFO) syscall failed: %s",
             mach_error_string(error));
        goto error;
    }
    mach_port_deallocate(mach_task_self(), host_port);

    cpu_load_info = (processor_cpu_load_info_data_t *) info_array;

    for (i = 0; i < cpu_count; i++) {
        py_cputime = Py_BuildValue(
            "(dddd)",
            (double)cpu_load_info[i].cpu_ticks[CPU_STATE_USER] / CLK_TCK,
            (double)cpu_load_info[i].cpu_ticks[CPU_STATE_NICE] / CLK_TCK,
            (double)cpu_load_info[i].cpu_ticks[CPU_STATE_SYSTEM] / CLK_TCK,
            (double)cpu_load_info[i].cpu_ticks[CPU_STATE_IDLE] / CLK_TCK
        );
        if (!py_cputime)
            goto error;
        if (PyList_Append(py_retlist, py_cputime))
            goto error;
        Py_DECREF(py_cputime);
    }

    ret = vm_deallocate(mach_task_self(), (vm_address_t)info_array,
                        info_count * sizeof(int));
    if (ret != KERN_SUCCESS)
        PyErr_WarnEx(PyExc_RuntimeWarning, "vm_deallocate() failed", 2);
    return py_retlist;

error:
    Py_XDECREF(py_cputime);
    Py_DECREF(py_retlist);
    if (cpu_load_info != NULL) {
        ret = vm_deallocate(mach_task_self(), (vm_address_t)info_array,
                            info_count * sizeof(int));
        if (ret != KERN_SUCCESS)
            PyErr_WarnEx(PyExc_RuntimeWarning, "vm_deallocate() failed", 2);
    }
    return NULL;
}