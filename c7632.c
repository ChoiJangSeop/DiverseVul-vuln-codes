g_vfs_daemon_init (GVfsDaemon *daemon)
{
  GError *error;
  gint max_threads = 1; /* TODO: handle max threads */

  daemon->thread_pool = g_thread_pool_new (job_handler_callback,
					   daemon,
					   max_threads,
					   FALSE, NULL);
  /* TODO: verify thread_pool != NULL in a nicer way */
  g_assert (daemon->thread_pool != NULL);

  g_mutex_init (&daemon->lock);

  daemon->mount_counter = 0;
  
  daemon->jobs = NULL;
  daemon->registered_paths =
    g_hash_table_new_full (g_str_hash, g_str_equal,
			   g_free, (GDestroyNotify)registered_path_free);

  /* This is where we store active client connections so when a new filter is registered,
   * we re-register them on all active connections */
  daemon->client_connections =
    g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, NULL);

  daemon->conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  g_assert (daemon->conn != NULL);

  daemon->daemon_skeleton = gvfs_dbus_daemon_skeleton_new ();
  g_signal_connect (daemon->daemon_skeleton, "handle-get-connection", G_CALLBACK (handle_get_connection), daemon);
  g_signal_connect (daemon->daemon_skeleton, "handle-cancel", G_CALLBACK (handle_cancel), daemon);
  g_signal_connect (daemon->daemon_skeleton, "handle-list-monitor-implementations", G_CALLBACK (handle_list_monitor_implementations), daemon);
  
  error = NULL;
  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (daemon->daemon_skeleton),
                                         daemon->conn,
                                         G_VFS_DBUS_DAEMON_PATH,
                                         &error))
    {
      g_warning ("Error exporting daemon interface: %s (%s, %d)\n",
                  error->message, g_quark_to_string (error->domain), error->code);
      g_error_free (error);
    }

  daemon->mountable_skeleton = gvfs_dbus_mountable_skeleton_new ();
  g_signal_connect (daemon->mountable_skeleton, "handle-mount", G_CALLBACK (daemon_handle_mount), daemon);
  
  error = NULL;
  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (daemon->mountable_skeleton),
      daemon->conn,
                                         G_VFS_DBUS_MOUNTABLE_PATH,
                                         &error))
    {
      g_warning ("Error exporting mountable interface: %s (%s, %d)\n",
                  error->message, g_quark_to_string (error->domain), error->code);
      g_error_free (error);
    }
}