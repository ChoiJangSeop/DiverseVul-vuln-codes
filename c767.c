chansrv_cleanup(int pid)
{
  char text[256];

  g_snprintf(text, 255, "/tmp/xrdp_chansrv_%8.8x_main_term", pid);
  if (g_file_exist(text))
  {
    g_file_delete(text);
  }
  g_snprintf(text, 255, "/tmp/xrdp_chansrv_%8.8x_thread_done", pid);
  if (g_file_exist(text))
  {
    g_file_delete(text);
  }
  return 0;
}