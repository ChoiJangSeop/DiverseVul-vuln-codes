static COMMANDS *find_command(char *name)
{
  uint len;
  char *end;
  DBUG_ENTER("find_command");

  DBUG_ASSERT(name != NULL);
  DBUG_PRINT("enter", ("name: '%s'", name));

  while (my_isspace(charset_info, *name))
    name++;
  /*
    If there is an \\g in the row or if the row has a delimiter but
    this is not a delimiter command, let add_line() take care of
    parsing the row and calling find_command().
  */
  if ((!real_binary_mode && strstr(name, "\\g")) ||
      (strstr(name, delimiter) &&
       !is_delimiter_command(name, DELIMITER_NAME_LEN)))
      DBUG_RETURN((COMMANDS *) 0);

  if ((end=strcont(name, " \t")))
  {
    len=(uint) (end - name);
    while (my_isspace(charset_info, *end))
      end++;
    if (!*end)
      end= 0;					// no arguments to function
  }
  else
    len= (uint) strlen(name);

  int index= -1;
  if (real_binary_mode)
  {
    if (is_delimiter_command(name, len))
      index= delimiter_index;
  }
  else
  {
    /*
      All commands are in the first part of commands array and have a function
      to implement it.
    */
    for (uint i= 0; commands[i].func; i++)
    {
      if (!my_strnncoll(&my_charset_latin1, (uchar*) name, len,
                        (uchar*) commands[i].name, len) &&
          (commands[i].name[len] == '\0') &&
          (!end || commands[i].takes_params))
      {
        index= i;
        break;
      }
    }
  }

  if (index >= 0)
  {
    DBUG_PRINT("exit", ("found command: %s", commands[index].name));
    DBUG_RETURN(&commands[index]);
  }
  DBUG_RETURN((COMMANDS *) 0);
}