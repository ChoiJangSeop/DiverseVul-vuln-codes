setup_listen(void)
{
  char port[256];
  int error = 0;

  if (g_lis_trans != 0)
  {
    trans_delete(g_lis_trans);
  }
  if (g_use_unix_socket)
  {
    g_lis_trans = trans_create(2, 8192, 8192);
    g_snprintf(port, 255, "/tmp/xrdp_chansrv_socket_%d", 7200 + g_display_num);
  }
  else
  {
    g_lis_trans = trans_create(1, 8192, 8192);
    g_snprintf(port, 255, "%d", 7200 + g_display_num);
  }
  g_lis_trans->trans_conn_in = my_trans_conn_in;
  error = trans_listen(g_lis_trans, port);
  if (error != 0)
  {
    LOG(0, ("setup_listen: trans_listen failed for port %s", port));
    return 1;
  }
  return 0;
}