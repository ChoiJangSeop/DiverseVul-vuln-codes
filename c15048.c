static char *utf2char(utf16_t *str, char *buffer)
{
  char *b = buffer;

  while (*str != 0x00) {
    char *chr = (char *)str;
    *b++ = *chr;
    str++;
  }
  *b = 0;
  return buffer;
}