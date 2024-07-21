flatpak_run_apply_env_vars (FlatpakBwrap *bwrap, FlatpakContext *context)
{
  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, context->env_vars);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      const char *var = key;
      const char *val = value;

      if (val && val[0] != 0)
        flatpak_bwrap_set_env (bwrap, var, val, TRUE);
      else
        flatpak_bwrap_unset_env (bwrap, var);
    }
}