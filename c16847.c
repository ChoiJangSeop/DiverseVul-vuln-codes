com_charset(String *buffer __attribute__((unused)), char *line)
{
  char buff[256], *param;
  CHARSET_INFO * new_cs;
  strmake_buf(buff, line);
  param= get_arg(buff, 0);
  if (!param || !*param)
  {
    return put_info("Usage: \\C charset_name | charset charset_name", 
		    INFO_ERROR, 0);
  }
  new_cs= get_charset_by_csname(param, MY_CS_PRIMARY, MYF(MY_WME));
  if (new_cs)
  {
    charset_info= new_cs;
    mysql_set_character_set(&mysql, charset_info->csname);
    default_charset= (char *)charset_info->csname;
    put_info("Charset changed", INFO_INFO);
  }
  else put_info("Charset is not found", INFO_INFO);
  return 0;
}