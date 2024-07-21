int MkvReader::Read(long long offset, long len, unsigned char* buffer) {
  if (m_file == NULL)
    return -1;

  if (offset < 0)
    return -1;

  if (len < 0)
    return -1;

  if (len == 0)
    return 0;

  if (offset >= m_length)
    return -1;

#ifdef _MSC_VER
  const int status = _fseeki64(m_file, offset, SEEK_SET);

  if (status)
    return -1;  // error
#else
  fseeko(m_file, static_cast<off_t>(offset), SEEK_SET);
#endif

  const size_t size = fread(buffer, 1, len, m_file);

  if (size < size_t(len))
    return -1;  // error

  return 0;  // success
}