g_vfs_daemon_finalize (GObject *object)
{
  GVfsDaemon *daemon;

  daemon = G_VFS_DAEMON (object);

  /* There may be some jobs outstanding if we've been force unmounted. */
  if (daemon->jobs)
    g_warning ("daemon->jobs != NULL when finalizing daemon!");

  if (daemon->name_watcher)
    g_bus_unwatch_name (daemon->name_watcher);
  
  if (daemon->daemon_skeleton != NULL)
    {
      g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (daemon->daemon_skeleton));
      g_object_unref (daemon->daemon_skeleton);
    }
  if (daemon->mountable_skeleton != NULL)
    {
      g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (daemon->mountable_skeleton));
      g_object_unref (daemon->mountable_skeleton);
    }
  if (daemon->conn != NULL)
    g_object_unref (daemon->conn);
  
  g_hash_table_destroy (daemon->registered_paths);
  g_hash_table_destroy (daemon->client_connections);
  g_mutex_clear (&daemon->lock);

  if (G_OBJECT_CLASS (g_vfs_daemon_parent_class)->finalize)
    (*G_OBJECT_CLASS (g_vfs_daemon_parent_class)->finalize) (object);
}