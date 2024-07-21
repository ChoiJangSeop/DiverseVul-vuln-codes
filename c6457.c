mysql_list_fields(MYSQL *mysql, const char *table, const char *wild)
{
  MYSQL_RES *result;
  MYSQL_DATA *query;
  char buff[255];
  int length= 0;

  LINT_INIT(query);

  length= snprintf(buff, 128, "%s%c%s", table, '\0', wild ? wild : "");

  if (ma_simple_command(mysql, COM_FIELD_LIST,buff,length,1,0) ||
      !(query = mysql->methods->db_read_rows(mysql,(MYSQL_FIELD*) 0,8)))
    return(NULL);

  free_old_query(mysql);
  if (!(result = (MYSQL_RES *) calloc(1, sizeof(MYSQL_RES))))
  {
    free_rows(query);
    return(NULL);
  }
  result->field_alloc=mysql->field_alloc;
  mysql->fields=0;
  result->field_count = (uint) query->rows;
  result->fields= unpack_fields(query,&result->field_alloc,
				result->field_count,1,
				(my_bool) test(mysql->server_capabilities &
					       CLIENT_LONG_FLAG));
  result->eof=1;
  return(result);
}