g_deinit(void)
{
#if defined(_WIN32)
  WSACleanup();
#endif
  g_remove_dir(g_temp_base);
}