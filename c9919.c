flatpak_context_load_metadata (FlatpakContext *context,
                               GKeyFile       *metakey,
                               GError        **error)
{
  gboolean remove;
  g_auto(GStrv) groups = NULL;
  gsize i;

  if (g_key_file_has_key (metakey, FLATPAK_METADATA_GROUP_CONTEXT, FLATPAK_METADATA_KEY_SHARED, NULL))
    {
      g_auto(GStrv) shares = g_key_file_get_string_list (metakey, FLATPAK_METADATA_GROUP_CONTEXT,
                                                         FLATPAK_METADATA_KEY_SHARED, NULL, error);
      if (shares == NULL)
        return FALSE;

      for (i = 0; shares[i] != NULL; i++)
        {
          FlatpakContextShares share;

          share = flatpak_context_share_from_string (parse_negated (shares[i], &remove), NULL);
          if (share == 0)
            g_debug ("Unknown share type %s", shares[i]);
          else
            {
              if (remove)
                flatpak_context_remove_shares (context, share);
              else
                flatpak_context_add_shares (context, share);
            }
        }
    }

  if (g_key_file_has_key (metakey, FLATPAK_METADATA_GROUP_CONTEXT, FLATPAK_METADATA_KEY_SOCKETS, NULL))
    {
      g_auto(GStrv) sockets = g_key_file_get_string_list (metakey, FLATPAK_METADATA_GROUP_CONTEXT,
                                                          FLATPAK_METADATA_KEY_SOCKETS, NULL, error);
      if (sockets == NULL)
        return FALSE;

      for (i = 0; sockets[i] != NULL; i++)
        {
          FlatpakContextSockets socket = flatpak_context_socket_from_string (parse_negated (sockets[i], &remove), NULL);
          if (socket == 0)
            g_debug ("Unknown socket type %s", sockets[i]);
          else
            {
              if (remove)
                flatpak_context_remove_sockets (context, socket);
              else
                flatpak_context_add_sockets (context, socket);
            }
        }
    }

  if (g_key_file_has_key (metakey, FLATPAK_METADATA_GROUP_CONTEXT, FLATPAK_METADATA_KEY_DEVICES, NULL))
    {
      g_auto(GStrv) devices = g_key_file_get_string_list (metakey, FLATPAK_METADATA_GROUP_CONTEXT,
                                                          FLATPAK_METADATA_KEY_DEVICES, NULL, error);
      if (devices == NULL)
        return FALSE;


      for (i = 0; devices[i] != NULL; i++)
        {
          FlatpakContextDevices device = flatpak_context_device_from_string (parse_negated (devices[i], &remove), NULL);
          if (device == 0)
            g_debug ("Unknown device type %s", devices[i]);
          else
            {
              if (remove)
                flatpak_context_remove_devices (context, device);
              else
                flatpak_context_add_devices (context, device);
            }
        }
    }

  if (g_key_file_has_key (metakey, FLATPAK_METADATA_GROUP_CONTEXT, FLATPAK_METADATA_KEY_FEATURES, NULL))
    {
      g_auto(GStrv) features = g_key_file_get_string_list (metakey, FLATPAK_METADATA_GROUP_CONTEXT,
                                                           FLATPAK_METADATA_KEY_FEATURES, NULL, error);
      if (features == NULL)
        return FALSE;


      for (i = 0; features[i] != NULL; i++)
        {
          FlatpakContextFeatures feature = flatpak_context_feature_from_string (parse_negated (features[i], &remove), NULL);
          if (feature == 0)
            g_debug ("Unknown feature type %s", features[i]);
          else
            {
              if (remove)
                flatpak_context_remove_features (context, feature);
              else
                flatpak_context_add_features (context, feature);
            }
        }
    }

  if (g_key_file_has_key (metakey, FLATPAK_METADATA_GROUP_CONTEXT, FLATPAK_METADATA_KEY_FILESYSTEMS, NULL))
    {
      g_auto(GStrv) filesystems = g_key_file_get_string_list (metakey, FLATPAK_METADATA_GROUP_CONTEXT,
                                                              FLATPAK_METADATA_KEY_FILESYSTEMS, NULL, error);
      if (filesystems == NULL)
        return FALSE;

      for (i = 0; filesystems[i] != NULL; i++)
        {
          const char *fs = parse_negated (filesystems[i], &remove);
          g_autofree char *filesystem = NULL;
          FlatpakFilesystemMode mode;

          if (!flatpak_context_parse_filesystem (fs, &filesystem, &mode, NULL))
            g_debug ("Unknown filesystem type %s", filesystems[i]);
          else
            {
              if (remove)
                flatpak_context_take_filesystem (context, g_steal_pointer (&filesystem),
                                                 FLATPAK_FILESYSTEM_MODE_NONE);
              else
                flatpak_context_take_filesystem (context, g_steal_pointer (&filesystem), mode);
            }
        }
    }

  if (g_key_file_has_key (metakey, FLATPAK_METADATA_GROUP_CONTEXT, FLATPAK_METADATA_KEY_PERSISTENT, NULL))
    {
      g_auto(GStrv) persistent = g_key_file_get_string_list (metakey, FLATPAK_METADATA_GROUP_CONTEXT,
                                                             FLATPAK_METADATA_KEY_PERSISTENT, NULL, error);
      if (persistent == NULL)
        return FALSE;

      for (i = 0; persistent[i] != NULL; i++)
        flatpak_context_set_persistent (context, persistent[i]);
    }

  if (g_key_file_has_group (metakey, FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY))
    {
      g_auto(GStrv) keys = NULL;
      gsize keys_count;

      keys = g_key_file_get_keys (metakey, FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY, &keys_count, NULL);
      for (i = 0; i < keys_count; i++)
        {
          const char *key = keys[i];
          g_autofree char *value = g_key_file_get_string (metakey, FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY, key, NULL);
          FlatpakPolicy policy;

          if (!flatpak_verify_dbus_name (key, error))
            return FALSE;

          policy = flatpak_policy_from_string (value, NULL);
          if ((int) policy != -1)
            flatpak_context_set_session_bus_policy (context, key, policy);
        }
    }

  if (g_key_file_has_group (metakey, FLATPAK_METADATA_GROUP_SYSTEM_BUS_POLICY))
    {
      g_auto(GStrv) keys = NULL;
      gsize keys_count;

      keys = g_key_file_get_keys (metakey, FLATPAK_METADATA_GROUP_SYSTEM_BUS_POLICY, &keys_count, NULL);
      for (i = 0; i < keys_count; i++)
        {
          const char *key = keys[i];
          g_autofree char *value = g_key_file_get_string (metakey, FLATPAK_METADATA_GROUP_SYSTEM_BUS_POLICY, key, NULL);
          FlatpakPolicy policy;

          if (!flatpak_verify_dbus_name (key, error))
            return FALSE;

          policy = flatpak_policy_from_string (value, NULL);
          if ((int) policy != -1)
            flatpak_context_set_system_bus_policy (context, key, policy);
        }
    }

  if (g_key_file_has_group (metakey, FLATPAK_METADATA_GROUP_ENVIRONMENT))
    {
      g_auto(GStrv) keys = NULL;
      gsize keys_count;

      keys = g_key_file_get_keys (metakey, FLATPAK_METADATA_GROUP_ENVIRONMENT, &keys_count, NULL);
      for (i = 0; i < keys_count; i++)
        {
          const char *key = keys[i];
          g_autofree char *value = g_key_file_get_string (metakey, FLATPAK_METADATA_GROUP_ENVIRONMENT, key, NULL);

          flatpak_context_set_env_var (context, key, value);
        }
    }

  groups = g_key_file_get_groups (metakey, NULL);
  for (i = 0; groups[i] != NULL; i++)
    {
      const char *group = groups[i];
      const char *subsystem;
      int j;

      if (g_str_has_prefix (group, FLATPAK_METADATA_GROUP_PREFIX_POLICY))
        {
          g_auto(GStrv) keys = NULL;
          subsystem = group + strlen (FLATPAK_METADATA_GROUP_PREFIX_POLICY);
          keys = g_key_file_get_keys (metakey, group, NULL, NULL);
          for (j = 0; keys != NULL && keys[j] != NULL; j++)
            {
              const char *key = keys[j];
              g_autofree char *policy_key = g_strdup_printf ("%s.%s", subsystem, key);
              g_auto(GStrv) values = NULL;
              int k;

              values = g_key_file_get_string_list (metakey, group, key, NULL, NULL);
              for (k = 0; values != NULL && values[k] != NULL; k++)
                flatpak_context_apply_generic_policy (context, policy_key,
                                                      values[k]);
            }
        }
    }

  return TRUE;
}