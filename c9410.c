UTI_GetRandomBytesUrandom(void *buf, unsigned int len)
{
  static FILE *f = NULL;

  if (!f)
    f = fopen(DEV_URANDOM, "r");
  if (!f)
    LOG_FATAL("Can't open %s : %s", DEV_URANDOM, strerror(errno));
  if (fread(buf, 1, len, f) != len)
    LOG_FATAL("Can't read from %s", DEV_URANDOM);
}