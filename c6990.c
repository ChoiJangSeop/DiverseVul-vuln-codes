_PyMemoTable_ResizeTable(PyMemoTable *self, Py_ssize_t min_size)
{
    PyMemoEntry *oldtable = NULL;
    PyMemoEntry *oldentry, *newentry;
    Py_ssize_t new_size = MT_MINSIZE;
    Py_ssize_t to_process;

    assert(min_size > 0);

    /* Find the smallest valid table size >= min_size. */
    while (new_size < min_size && new_size > 0)
        new_size <<= 1;
    if (new_size <= 0) {
        PyErr_NoMemory();
        return -1;
    }
    /* new_size needs to be a power of two. */
    assert((new_size & (new_size - 1)) == 0);

    /* Allocate new table. */
    oldtable = self->mt_table;
    self->mt_table = PyMem_NEW(PyMemoEntry, new_size);
    if (self->mt_table == NULL) {
        self->mt_table = oldtable;
        PyErr_NoMemory();
        return -1;
    }
    self->mt_allocated = new_size;
    self->mt_mask = new_size - 1;
    memset(self->mt_table, 0, sizeof(PyMemoEntry) * new_size);

    /* Copy entries from the old table. */
    to_process = self->mt_used;
    for (oldentry = oldtable; to_process > 0; oldentry++) {
        if (oldentry->me_key != NULL) {
            to_process--;
            /* newentry is a pointer to a chunk of the new
               mt_table, so we're setting the key:value pair
               in-place. */
            newentry = _PyMemoTable_Lookup(self, oldentry->me_key);
            newentry->me_key = oldentry->me_key;
            newentry->me_value = oldentry->me_value;
        }
    }

    /* Deallocate the old table. */
    PyMem_FREE(oldtable);
    return 0;
}