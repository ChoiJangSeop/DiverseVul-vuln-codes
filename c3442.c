list_fields(MYSQL *mysql,const char *db,const char *table,
	    const char *wild)
{
  char query[1024],*end;
  MYSQL_RES *result;
  MYSQL_ROW row;
  ulong UNINIT_VAR(rows);

  if (mysql_select_db(mysql,db))
  {
    fprintf(stderr,"%s: Cannot connect to db: %s: %s\n",my_progname,db,
	    mysql_error(mysql));
    return 1;
  }

  if (opt_count)
  {
    sprintf(query,"select count(*) from `%s`", table);
    if (mysql_query(mysql,query) || !(result=mysql_store_result(mysql)))
    {
      fprintf(stderr,"%s: Cannot get record count for db: %s, table: %s: %s\n",
              my_progname,db,table,mysql_error(mysql));
      return 1;
    }
    row= mysql_fetch_row(result);
    rows= (ulong) strtoull(row[0], (char**) 0, 10);
    mysql_free_result(result);
  }

  end=strmov(strmov(strmov(query,"show /*!32332 FULL */ columns from `"),table),"`");
  if (wild && wild[0])
    strxmov(end," like '",wild,"'",NullS);
  if (mysql_query(mysql,query) || !(result=mysql_store_result(mysql)))
  {
    fprintf(stderr,"%s: Cannot list columns in db: %s, table: %s: %s\n",
	    my_progname,db,table,mysql_error(mysql));
    return 1;
  }

  printf("Database: %s  Table: %s", db, table);
  if (opt_count)
    printf("  Rows: %lu", rows);
  if (wild && wild[0])
    printf("  Wildcard: %s",wild);
  putchar('\n');

  print_res_header(result);
  while ((row=mysql_fetch_row(result)))
    print_res_row(result,row);
  print_res_top(result);
  if (opt_show_keys)
  {
    end=strmov(strmov(strmov(query,"show keys from `"),table),"`");
    if (mysql_query(mysql,query) || !(result=mysql_store_result(mysql)))
    {
      fprintf(stderr,"%s: Cannot list keys in db: %s, table: %s: %s\n",
	      my_progname,db,table,mysql_error(mysql));
      return 1;
    }
    if (mysql_num_rows(result))
    {
      print_res_header(result);
      while ((row=mysql_fetch_row(result)))
	print_res_row(result,row);
      print_res_top(result);
    }
    else
      puts("Table has no keys");
  }
  mysql_free_result(result);
  return 0;
}