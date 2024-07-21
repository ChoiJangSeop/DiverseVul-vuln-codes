void _mutt_mktemp (char *s, size_t slen, const char *src, int line)
{
  snprintf (s, slen, "%s/mutt-%s-%d-%d-%d", NONULL (Tempdir), NONULL(Hostname), (int) getuid(), (int) getpid (), Counter++);
  dprint (3, (debugfile, "%s:%d: mutt_mktemp returns \"%s\".\n", src, line, s));
  unlink (s);
}