list_dbs(MYSQL *mysql,const char *wild)
{
  const char *header;
  uint length, counter = 0;
  ulong rowcount = 0L;
  char tables[NAME_LEN+1], rows[NAME_LEN+1];
  char query[255];
  MYSQL_FIELD *field;
  MYSQL_RES *result;
  MYSQL_ROW row= NULL, rrow;

  if (!(result=mysql_list_dbs(mysql,wild)))
  {
    fprintf(stderr,"%s: Cannot list databases: %s\n",my_progname,
	    mysql_error(mysql));
    return 1;
  }

  /*
    If a wildcard was used, but there was only one row and it's name is an
    exact match, we'll assume they really wanted to see the contents of that
    database. This is because it is fairly common for database names to
    contain the underscore (_), like INFORMATION_SCHEMA.
   */
  if (wild && mysql_num_rows(result) == 1)
  {
    row= mysql_fetch_row(result);
    if (!my_strcasecmp(&my_charset_latin1, row[0], wild))
    {
      mysql_free_result(result);
      if (opt_status)
        return list_table_status(mysql, wild, NULL);
      else
        return list_tables(mysql, wild, NULL);
    }
  }

  if (wild)
    printf("Wildcard: %s\n",wild);

  header="Databases";
  length=(uint) strlen(header);
  field=mysql_fetch_field(result);
  if (length < field->max_length)
    length=field->max_length;

  if (!opt_verbose)
    print_header(header,length,NullS);
  else if (opt_verbose == 1)
    print_header(header,length,"Tables",6,NullS);
  else
    print_header(header,length,"Tables",6,"Total Rows",12,NullS);

  /* The first row may have already been read up above. */
  while (row || (row= mysql_fetch_row(result)))
  {
    counter++;

    if (opt_verbose)
    {
      if (!(mysql_select_db(mysql,row[0])))
      {
	MYSQL_RES *tresult = mysql_list_tables(mysql,(char*)NULL);
	if (mysql_affected_rows(mysql) > 0)
	{
	  sprintf(tables,"%6lu",(ulong) mysql_affected_rows(mysql));
	  rowcount = 0;
	  if (opt_verbose > 1)
	  {
            /* Print the count of tables and rows for each database */
            MYSQL_ROW trow;
	    while ((trow = mysql_fetch_row(tresult)))
	    {
	      sprintf(query,"SELECT COUNT(*) FROM `%s`",trow[0]);
	      if (!(mysql_query(mysql,query)))
	      {
		MYSQL_RES *rresult;
		if ((rresult = mysql_store_result(mysql)))
		{
		  rrow = mysql_fetch_row(rresult);
		  rowcount += (ulong) strtoull(rrow[0], (char**) 0, 10);
		  mysql_free_result(rresult);
		}
	      }
	    }
	    sprintf(rows,"%12lu",rowcount);
	  }
	}
	else
	{
	  sprintf(tables,"%6d",0);
	  sprintf(rows,"%12d",0);
	}
	mysql_free_result(tresult);
      }
      else
      {
	strmov(tables,"N/A");
	strmov(rows,"N/A");
      }
    }

    if (!opt_verbose)
      print_row(row[0],length,0);
    else if (opt_verbose == 1)
      print_row(row[0],length,tables,6,NullS);
    else
      print_row(row[0],length,tables,6,rows,12,NullS);

    row= NULL;
  }

  print_trailer(length,
		(opt_verbose > 0 ? 6 : 0),
		(opt_verbose > 1 ? 12 :0),
		0);

  if (counter && opt_verbose)
    printf("%u row%s in set.\n",counter,(counter > 1) ? "s" : "");
  mysql_free_result(result);
  return 0;
}