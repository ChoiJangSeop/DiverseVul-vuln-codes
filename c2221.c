static char *memdup(const char *src, size_t buffer_length)
{
  size_t length;
  bool add = FALSE;
  char *buffer;

  if(buffer_length)
    length = buffer_length;
  else if(src) {
    length = strlen(src);
    add = TRUE;
  }
  else
    /* no length and a NULL src pointer! */
    return strdup("");

  buffer = malloc(length+add);
  if(!buffer)
    return NULL; /* fail */

  memcpy(buffer, src, length);

  /* if length unknown do null termination */
  if(add)
    buffer[length] = '\0';

  return buffer;
}