iobuf_temp_with_content (const char *buffer, size_t length)
{
  iobuf_t a;

  a = iobuf_alloc (3, length);
  memcpy (a->d.buf, buffer, length);
  a->d.len = length;

  return a;
}