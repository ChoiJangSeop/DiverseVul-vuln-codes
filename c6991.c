_pickle_PicklerMemoProxy_copy_impl(PicklerMemoProxyObject *self)
/*[clinic end generated code: output=bb83a919d29225ef input=b73043485ac30b36]*/
{
    Py_ssize_t i;
    PyMemoTable *memo;
    PyObject *new_memo = PyDict_New();
    if (new_memo == NULL)
        return NULL;

    memo = self->pickler->memo;
    for (i = 0; i < memo->mt_allocated; ++i) {
        PyMemoEntry entry = memo->mt_table[i];
        if (entry.me_key != NULL) {
            int status;
            PyObject *key, *value;

            key = PyLong_FromVoidPtr(entry.me_key);
            value = Py_BuildValue("nO", entry.me_value, entry.me_key);

            if (key == NULL || value == NULL) {
                Py_XDECREF(key);
                Py_XDECREF(value);
                goto error;
            }
            status = PyDict_SetItem(new_memo, key, value);
            Py_DECREF(key);
            Py_DECREF(value);
            if (status < 0)
                goto error;
        }
    }
    return new_memo;

  error:
    Py_XDECREF(new_memo);
    return NULL;
}