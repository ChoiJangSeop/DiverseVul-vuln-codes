int32 MkvWriter::Position(int64 position) {
  if (!file_)
    return -1;

#ifdef _MSC_VER
  return _fseeki64(file_, position, SEEK_SET);
#else
  return fseeko(file_, static_cast<off_t>(position), SEEK_SET);
#endif
}