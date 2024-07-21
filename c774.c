env_set_user(char* username, char* passwd_file, int display)
{
  int error;
  int pw_uid;
  int pw_gid;
  int uid;
  char pw_shell[256];
  char pw_dir[256];
  char pw_gecos[256];
  char text[256];

  error = g_getuser_info(username, &pw_gid, &pw_uid, pw_shell, pw_dir,
                         pw_gecos);
  if (error == 0)
  {
    error = g_setgid(pw_gid);
    if (error == 0)
    {
      error = g_initgroups(username, pw_gid);
    }
    if (error == 0)
    {
      uid = pw_uid;
      error = g_setuid(uid);
    }
    if (error == 0)
    {
      g_clearenv();
      g_setenv("SHELL", pw_shell, 1);
      g_setenv("PATH", "/bin:/usr/bin:/usr/X11R6/bin:/usr/local/bin", 1);
      g_setenv("USER", username, 1);
      g_sprintf(text, "%d", uid);
      g_setenv("UID", text, 1);
      g_setenv("HOME", pw_dir, 1);
      g_set_current_dir(pw_dir);
      g_sprintf(text, ":%d.0", display);
      g_setenv("DISPLAY", text, 1);
      if (passwd_file != 0)
      {
        if (0 == g_cfg->auth_file_path)
        {
          /* if no auth_file_path is set, then we go for
             $HOME/.vnc/sesman_username_passwd */
          g_mkdir(".vnc");
          g_sprintf(passwd_file, "%s/.vnc/sesman_%s_passwd", pw_dir, username);
        }
        else
        {
          /* we use auth_file_path as requested */
          g_sprintf(passwd_file, g_cfg->auth_file_path, username);
        }
        LOG_DBG(&(g_cfg->log), "pass file: %s", passwd_file);
      }
    }
  }
  else
  {
    log_message(&(g_cfg->log), LOG_LEVEL_ERROR,
                "error getting user info for user %s", username);
  }
  return error;
}