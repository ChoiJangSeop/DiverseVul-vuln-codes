PJ_DEF(pj_status_t) pj_file_setpos( pj_oshandle_t fd,
                                    pj_off_t offset,
                                    enum pj_file_seek_type whence)
{
    int mode;

    switch (whence) {
    case PJ_SEEK_SET:
        mode = SEEK_SET; break;
    case PJ_SEEK_CUR:
        mode = SEEK_CUR; break;
    case PJ_SEEK_END:
        mode = SEEK_END; break;
    default:
        pj_assert(!"Invalid whence in file_setpos");
        return PJ_EINVAL;
    }

    if (fseek((FILE*)fd, (long)offset, mode) != 0)
        return PJ_RETURN_OS_ERROR(errno);

    return PJ_SUCCESS;
}