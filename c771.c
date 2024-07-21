g_execvp(const char* p1, char* args[])
{
#if defined(_WIN32)
  return 0;
#else
  return execvp(p1, args);
#endif
}