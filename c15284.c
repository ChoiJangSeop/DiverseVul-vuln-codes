QUtil::seek(FILE* stream, qpdf_offset_t offset, int whence)
{
#if HAVE_FSEEKO
    return fseeko(stream,
                  QIntC::IntConverter<qpdf_offset_t, off_t>::convert(offset),
                  whence);
#elif HAVE_FSEEKO64
    return fseeko64(stream, offset, whence);
#else
# if defined _MSC_VER || defined __BORLANDC__
    return _fseeki64(stream, offset, whence);
# else
    return fseek(stream,
                 QIntC::IntConverter<qpdf_offset_t, long>(offset),
                 whence);
# endif
#endif
}