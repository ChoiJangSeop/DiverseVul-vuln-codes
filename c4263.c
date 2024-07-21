static bool check_log_path(sys_var *self, THD *thd, set_var *var)
{
  if (!var->value)
    return false; // DEFAULT is ok

  if (!var->save_result.string_value.str)
    return true;

  if (var->save_result.string_value.length > FN_REFLEN)
  { // path is too long
    my_error(ER_PATH_LENGTH, MYF(0), self->name.str);
    return true;
  }

  char path[FN_REFLEN];
  size_t path_length= unpack_filename(path, var->save_result.string_value.str);

  if (!path_length)
    return true;

  if (!is_filename_allowed(var->save_result.string_value.str, 
                           var->save_result.string_value.length))
  {
     my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), 
              self->name.str, var->save_result.string_value.str);
     return true;
  }

  MY_STAT f_stat;

  if (my_stat(path, &f_stat, MYF(0)))
  {
    if (!MY_S_ISREG(f_stat.st_mode) || !(f_stat.st_mode & MY_S_IWRITE))
      return true; // not a regular writable file
    return false;
  }

  (void) dirname_part(path, var->save_result.string_value.str, &path_length);

  if (var->save_result.string_value.length - path_length >= FN_LEN)
  { // filename is too long
      my_error(ER_PATH_LENGTH, MYF(0), self->name.str);
      return true;
  }

  if (!path_length) // no path is good path (remember, relative to datadir)
    return false;

  if (my_access(path, (F_OK|W_OK)))
    return true; // directory is not writable

  return false;
}