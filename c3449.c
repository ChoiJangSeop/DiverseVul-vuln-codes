list_table_status(MYSQL *mysql,const char *db,const char *wild)
{
  char query[1024],*end;
  MYSQL_RES *result;
  MYSQL_ROW row;

  end=strxmov(query,"show table status from `",db,"`",NullS);
  if (wild && wild[0])
    strxmov(end," like '",wild,"'",NullS);
  if (mysql_query(mysql,query) || !(result=mysql_store_result(mysql)))
  {
    fprintf(stderr,"%s: Cannot get status for db: %s, table: %s: %s\n",
	    my_progname,db,wild ? wild : "",mysql_error(mysql));
    if (mysql_errno(mysql) == ER_PARSE_ERROR)
      fprintf(stderr,"This error probably means that your MySQL server doesn't support the\n\'show table status' command.\n");
    return 1;
  }

  printf("Database: %s",db);
  if (wild)
    printf("  Wildcard: %s",wild);
  putchar('\n');

  print_res_header(result);
  while ((row=mysql_fetch_row(result)))
    print_res_row(result,row);
  print_res_top(result);
  mysql_free_result(result);
  return 0;
}