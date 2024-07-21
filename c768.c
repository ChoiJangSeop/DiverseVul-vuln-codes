g_init(const char* app_name)
{
#if defined(_WIN32)
  WSADATA wsadata;

  WSAStartup(2, &wsadata);
#endif
  setlocale(LC_CTYPE, "");
  if (app_name != 0)
  {
    if (app_name[0] != 0)
    {
      snprintf(g_temp_base, sizeof(g_temp_base), "/tmp/%s-XXXXXX", app_name);
      if (mkdtemp(g_temp_base) == 0)
      {
        printf("g_init: mkdtemp failed [%s]\n", g_temp_base);
      }
    }
  }
}