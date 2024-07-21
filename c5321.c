com_use(String *buffer __attribute__((unused)), char *line)
{
  char *tmp, buff[FN_REFLEN + 1];
  int select_db;

  bzero(buff, sizeof(buff));

  /*
    In case number of quotes exceed 2, we try to get
    the normalized db name.
  */
  if (get_quote_count(line) > 2)
  {
    if (normalize_dbname(line, buff, sizeof(buff)))
      return put_error(&mysql);
    tmp= buff;
  }
  else
  {
    strmake(buff, line, sizeof(buff) - 1);
    tmp= get_arg(buff, 0);
  }

  if (!tmp || !*tmp)
  {
    put_info("USE must be followed by a database name", INFO_ERROR);
    return 0;
  }
  /*
    We need to recheck the current database, because it may change
    under our feet, for example if DROP DATABASE or RENAME DATABASE
    (latter one not yet available by the time the comment was written)
  */
  get_current_db();

  if (!current_db || cmp_database(charset_info, current_db,tmp))
  {
    if (one_database)
    {
      skip_updates= 1;
      select_db= 0;    // don't do mysql_select_db()
    }
    else
      select_db= 2;    // do mysql_select_db() and build_completion_hash()
  }
  else
  {
    /*
      USE to the current db specified.
      We do need to send mysql_select_db() to make server
      update database level privileges, which might
      change since last USE (see bug#10979).
      For performance purposes, we'll skip rebuilding of completion hash.
    */
    skip_updates= 0;
    select_db= 1;      // do only mysql_select_db(), without completion
  }

  if (select_db)
  {
    /*
      reconnect once if connection is down or if connection was found to
      be down during query
    */
    if (!connected && reconnect())
      return opt_reconnect ? -1 : 1;                        // Fatal error
    if (mysql_select_db(&mysql,tmp))
    {
      if (mysql_errno(&mysql) != CR_SERVER_GONE_ERROR)
        return put_error(&mysql);

      if (reconnect())
        return opt_reconnect ? -1 : 1;                      // Fatal error
      if (mysql_select_db(&mysql,tmp))
        return put_error(&mysql);
    }
    my_free(current_db);
    current_db=my_strdup(tmp,MYF(MY_WME));
#ifdef HAVE_READLINE
    if (select_db > 1)
      build_completion_hash(opt_rehash, 1);
#endif
  }

  put_info("Database changed",INFO_INFO);
  return 0;
}