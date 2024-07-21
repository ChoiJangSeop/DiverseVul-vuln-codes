list_tables(MYSQL *mysql,const char *db,const char *table)
{
  const char *header;
  uint head_length, counter = 0;
  char query[255], rows[NAME_LEN], fields[16];
  MYSQL_FIELD *field;
  MYSQL_RES *result;
  MYSQL_ROW row, rrow;

  if (mysql_select_db(mysql,db))
  {
    fprintf(stderr,"%s: Cannot connect to db %s: %s\n",my_progname,db,
	    mysql_error(mysql));
    return 1;
  }
  if (table)
  {
    /*
      We just hijack the 'rows' variable for a bit to store the escaped
      table name
    */
    mysql_real_escape_string(mysql, rows, table, (unsigned long)strlen(table));
    my_snprintf(query, sizeof(query), "show%s tables like '%s'",
                opt_table_type ? " full" : "", rows);
  }
  else
    my_snprintf(query, sizeof(query), "show%s tables",
                opt_table_type ? " full" : "");
  if (mysql_query(mysql, query) || !(result= mysql_store_result(mysql)))
  {
    fprintf(stderr,"%s: Cannot list tables in %s: %s\n",my_progname,db,
	    mysql_error(mysql));
    exit(1);
  }
  printf("Database: %s",db);
  if (table)
    printf("  Wildcard: %s",table);
  putchar('\n');

  header="Tables";
  head_length=(uint) strlen(header);
  field=mysql_fetch_field(result);
  if (head_length < field->max_length)
    head_length=field->max_length;

  if (opt_table_type)
  {
    if (!opt_verbose)
      print_header(header,head_length,"table_type",10,NullS);
    else if (opt_verbose == 1)
      print_header(header,head_length,"table_type",10,"Columns",8,NullS);
    else
    {
      print_header(header,head_length,"table_type",10,"Columns",8,
		   "Total Rows",10,NullS);
    }
  }
  else
  {
    if (!opt_verbose)
      print_header(header,head_length,NullS);
    else if (opt_verbose == 1)
      print_header(header,head_length,"Columns",8,NullS);
    else
      print_header(header,head_length,"Columns",8, "Total Rows",10,NullS);
  }

  while ((row = mysql_fetch_row(result)))
  {
    counter++;
    if (opt_verbose > 0)
    {
      if (!(mysql_select_db(mysql,db)))
      {
	MYSQL_RES *rresult = mysql_list_fields(mysql,row[0],NULL);
	ulong rowcount=0L;
	if (!rresult)
	{
	  strmov(fields,"N/A");
	  strmov(rows,"N/A");
	}
	else
	{
	  sprintf(fields,"%8u",(uint) mysql_num_fields(rresult));
	  mysql_free_result(rresult);

	  if (opt_verbose > 1)
	  {
            /* Print the count of rows for each table */
	    sprintf(query,"SELECT COUNT(*) FROM `%s`",row[0]);
	    if (!(mysql_query(mysql,query)))
	    {
	      if ((rresult = mysql_store_result(mysql)))
	      {
		rrow = mysql_fetch_row(rresult);
		rowcount += (unsigned long) strtoull(rrow[0], (char**) 0, 10);
		mysql_free_result(rresult);
	      }
	      sprintf(rows,"%10lu",rowcount);
	    }
	    else
	      sprintf(rows,"%10d",0);
	  }
	}
      }
      else
      {
	strmov(fields,"N/A");
	strmov(rows,"N/A");
      }
    }
    if (opt_table_type)
    {
      if (!opt_verbose)
	print_row(row[0],head_length,row[1],10,NullS);
      else if (opt_verbose == 1)
	print_row(row[0],head_length,row[1],10,fields,8,NullS);
      else
	print_row(row[0],head_length,row[1],10,fields,8,rows,10,NullS);
    }
    else
    {
      if (!opt_verbose)
	print_row(row[0],head_length,NullS);
      else if (opt_verbose == 1)
	print_row(row[0],head_length, fields,8, NullS);
      else
	print_row(row[0],head_length, fields,8, rows,10, NullS);
    }
  }

  print_trailer(head_length,
		(opt_table_type ? 10 : opt_verbose > 0 ? 8 : 0),
		(opt_table_type ? (opt_verbose > 0 ? 8 : 0) 
		 : (opt_verbose > 1 ? 10 :0)),
		!opt_table_type ? 0 : opt_verbose > 1 ? 10 :0,
		0);

  if (counter && opt_verbose)
    printf("%u row%s in set.\n\n",counter,(counter > 1) ? "s" : "");

  mysql_free_result(result);
  return 0;
}