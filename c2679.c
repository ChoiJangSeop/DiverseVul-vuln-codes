void dd_close(struct dump_dir *dd)
{
    if (!dd)
        return;

    dd_unlock(dd);
    if (dd->next_dir)
    {
        closedir(dd->next_dir);
        /* free(dd->next_dir); - WRONG! */
    }

    free(dd->dd_type);
    free(dd->dd_dirname);
    free(dd);
}