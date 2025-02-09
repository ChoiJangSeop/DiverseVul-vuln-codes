_pickle_UnpicklerMemoProxy_copy_impl(UnpicklerMemoProxyObject *self)
/*[clinic end generated code: output=e12af7e9bc1e4c77 input=97769247ce032c1d]*/
{
    Py_ssize_t i;
    PyObject *new_memo = PyDict_New();
    if (new_memo == NULL)
        return NULL;

    for (i = 0; i < self->unpickler->memo_size; i++) {
        int status;
        PyObject *key, *value;

        value = self->unpickler->memo[i];
        if (value == NULL)
            continue;

        key = PyLong_FromSsize_t(i);
        if (key == NULL)
            goto error;
        status = PyDict_SetItem(new_memo, key, value);
        Py_DECREF(key);
        if (status < 0)
            goto error;
    }
    return new_memo;

error:
    Py_DECREF(new_memo);
    return NULL;
}