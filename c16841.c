static int dump_tablespaces(char* ts_where)
{
  MYSQL_ROW row;
  MYSQL_RES *tableres;
  char buf[FN_REFLEN];
  DYNAMIC_STRING sqlbuf;
  int first= 0;
  /*
    The following are used for parsing the EXTRA field
  */
  char extra_format[]= "UNDO_BUFFER_SIZE=";
  char *ubs;
  char *endsemi;
  DBUG_ENTER("dump_tablespaces");
  
  /*
    Try to turn off semi-join optimization (if that fails, this is a
    pre-optimizer_switch server, and the old query plan is ok for us.
  */
  mysql_query(mysql, "set optimizer_switch='semijoin=off'");
 
  init_dynamic_string_checked(&sqlbuf,
                      "SELECT LOGFILE_GROUP_NAME,"
                      " FILE_NAME,"
                      " TOTAL_EXTENTS,"
                      " INITIAL_SIZE,"
                      " ENGINE,"
                      " EXTRA"
                      " FROM INFORMATION_SCHEMA.FILES"
                      " WHERE FILE_TYPE = 'UNDO LOG'"
                      " AND FILE_NAME IS NOT NULL",
                      256, 1024);
  if(ts_where)
  {
    dynstr_append_checked(&sqlbuf,
                  " AND LOGFILE_GROUP_NAME IN ("
                  "SELECT DISTINCT LOGFILE_GROUP_NAME"
                  " FROM INFORMATION_SCHEMA.FILES"
                  " WHERE FILE_TYPE = 'DATAFILE'"
                  );
    dynstr_append_checked(&sqlbuf, ts_where);
    dynstr_append_checked(&sqlbuf, ")");
  }
  dynstr_append_checked(&sqlbuf,
                " GROUP BY LOGFILE_GROUP_NAME, FILE_NAME"
                ", ENGINE"
                " ORDER BY LOGFILE_GROUP_NAME");

  if (mysql_query(mysql, sqlbuf.str) ||
      !(tableres = mysql_store_result(mysql)))
  {
    dynstr_free(&sqlbuf);
    if (mysql_errno(mysql) == ER_BAD_TABLE_ERROR ||
        mysql_errno(mysql) == ER_BAD_DB_ERROR ||
        mysql_errno(mysql) == ER_UNKNOWN_TABLE)
    {
      fprintf(md_result_file,
              "\n--\n-- Not dumping tablespaces as no INFORMATION_SCHEMA.FILES"
              " table on this server\n--\n");
      check_io(md_result_file);
      DBUG_RETURN(0);
    }

    fprintf(stderr, "%s: Error: '%s' when trying to dump tablespaces\n",
                    my_progname_short, mysql_error(mysql));
    DBUG_RETURN(1);
  }

  buf[0]= 0;
  while ((row= mysql_fetch_row(tableres)))
  {
    if (strcmp(buf, row[0]) != 0)
      first= 1;
    if (first)
    {
      print_comment(md_result_file, 0, "\n--\n-- Logfile group: %s\n--\n",
                    row[0]);

      fprintf(md_result_file, "\nCREATE");
    }
    else
    {
      fprintf(md_result_file, "\nALTER");
    }
    fprintf(md_result_file,
            " LOGFILE GROUP %s\n"
            "  ADD UNDOFILE '%s'\n",
            row[0],
            row[1]);
    if (first)
    {
      ubs= strstr(row[5],extra_format);
      if(!ubs)
        break;
      ubs+= strlen(extra_format);
      endsemi= strstr(ubs,";");
      if(endsemi)
        endsemi[0]= '\0';
      fprintf(md_result_file,
              "  UNDO_BUFFER_SIZE %s\n",
              ubs);
    }
    fprintf(md_result_file,
            "  INITIAL_SIZE %s\n"
            "  ENGINE=%s;\n",
            row[3],
            row[4]);
    check_io(md_result_file);
    if (first)
    {
      first= 0;
      strxmov(buf, row[0], NullS);
    }
  }
  dynstr_free(&sqlbuf);
  mysql_free_result(tableres);
  init_dynamic_string_checked(&sqlbuf,
                      "SELECT DISTINCT TABLESPACE_NAME,"
                      " FILE_NAME,"
                      " LOGFILE_GROUP_NAME,"
                      " EXTENT_SIZE,"
                      " INITIAL_SIZE,"
                      " ENGINE"
                      " FROM INFORMATION_SCHEMA.FILES"
                      " WHERE FILE_TYPE = 'DATAFILE'",
                      256, 1024);

  if(ts_where)
    dynstr_append_checked(&sqlbuf, ts_where);

  dynstr_append_checked(&sqlbuf, " ORDER BY TABLESPACE_NAME, LOGFILE_GROUP_NAME");

  if (mysql_query_with_error_report(mysql, &tableres, sqlbuf.str))
  {
    dynstr_free(&sqlbuf);
    DBUG_RETURN(1);
  }

  buf[0]= 0;
  while ((row= mysql_fetch_row(tableres)))
  {
    if (strcmp(buf, row[0]) != 0)
      first= 1;
    if (first)
    {
      print_comment(md_result_file, 0, "\n--\n-- Tablespace: %s\n--\n", row[0]);
      fprintf(md_result_file, "\nCREATE");
    }
    else
    {
      fprintf(md_result_file, "\nALTER");
    }
    fprintf(md_result_file,
            " TABLESPACE %s\n"
            "  ADD DATAFILE '%s'\n",
            row[0],
            row[1]);
    if (first)
    {
      fprintf(md_result_file,
              "  USE LOGFILE GROUP %s\n"
              "  EXTENT_SIZE %s\n",
              row[2],
              row[3]);
    }
    fprintf(md_result_file,
            "  INITIAL_SIZE %s\n"
            "  ENGINE=%s;\n",
            row[4],
            row[5]);
    check_io(md_result_file);
    if (first)
    {
      first= 0;
      strxmov(buf, row[0], NullS);
    }
  }

  mysql_free_result(tableres);
  dynstr_free(&sqlbuf);
  mysql_query(mysql, "set optimizer_switch=default");

  DBUG_RETURN(0);
}