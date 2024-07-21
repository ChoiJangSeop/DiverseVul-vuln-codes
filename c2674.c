int dd_reset_ownership(struct dump_dir *dd)
{
    const int r =lchown(dd->dd_dirname, dd->dd_uid, dd->dd_gid);
    if (r < 0)
    {
        perror_msg("Can't change '%s' ownership to %lu:%lu", dd->dd_dirname,
                   (long)dd->dd_uid, (long)dd->dd_gid);
    }
    return r;
}