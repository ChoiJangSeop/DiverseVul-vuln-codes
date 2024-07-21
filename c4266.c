static bool fix_log(char** logname, const char* default_logname,
                    const char*ext, bool enabled, void (*reopen)(char*))
{
  if (!*logname) // SET ... = DEFAULT
  {
    char buff[FN_REFLEN];
    *logname= my_strdup(make_log_name(buff, default_logname, ext),
                        MYF(MY_FAE+MY_WME));
    if (!*logname)
      return true;
  }
  logger.lock_exclusive();
  mysql_mutex_unlock(&LOCK_global_system_variables);
  if (enabled)
    reopen(*logname);
  logger.unlock();
  mysql_mutex_lock(&LOCK_global_system_variables);
  return false;
}