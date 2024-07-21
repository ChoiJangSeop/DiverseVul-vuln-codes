com_delimiter(String *buffer __attribute__((unused)), char *line)
{
  char buff[256], *tmp;

  strmake_buf(buff, line);
  tmp= get_arg(buff, 0);

  if (!tmp || !*tmp)
  {
    put_info("DELIMITER must be followed by a 'delimiter' character or string",
	     INFO_ERROR);
    return 0;
  }
  else
  {
    if (strstr(tmp, "\\")) 
    {
      put_info("DELIMITER cannot contain a backslash character", INFO_ERROR);
      return 0;
    }
  }
  strmake_buf(delimiter, tmp);
  delimiter_length= (int)strlen(delimiter);
  delimiter_str= delimiter;
  return 0;
}