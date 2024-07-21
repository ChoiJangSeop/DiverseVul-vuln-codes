static uint get_table_structure(char *table, char *db, char *table_type,
                                char *ignore_flag)
{
  my_bool    init=0, delayed, write_data, complete_insert;
  my_ulonglong num_fields;
  char       *result_table, *opt_quoted_table;
  const char *insert_option;
  char	     name_buff[NAME_LEN+3],table_buff[NAME_LEN*2+3];
  char       table_buff2[NAME_LEN*2+3], query_buff[QUERY_LENGTH];
  const char *show_fields_stmt= "SELECT `COLUMN_NAME` AS `Field`, "
                                "`COLUMN_TYPE` AS `Type`, "
                                "`IS_NULLABLE` AS `Null`, "
                                "`COLUMN_KEY` AS `Key`, "
                                "`COLUMN_DEFAULT` AS `Default`, "
                                "`EXTRA` AS `Extra`, "
                                "`COLUMN_COMMENT` AS `Comment` "
                                "FROM `INFORMATION_SCHEMA`.`COLUMNS` WHERE "
                                "TABLE_SCHEMA = '%s' AND TABLE_NAME = '%s'";
  FILE       *sql_file= md_result_file;
  int        len;
  my_bool    is_log_table;
  MYSQL_RES  *result;
  MYSQL_ROW  row;
  DBUG_ENTER("get_table_structure");
  DBUG_PRINT("enter", ("db: %s  table: %s", db, table));

  *ignore_flag= check_if_ignore_table(table, table_type);

  delayed= opt_delayed;
  if (delayed && (*ignore_flag & IGNORE_INSERT_DELAYED))
  {
    delayed= 0;
    verbose_msg("-- Warning: Unable to use delayed inserts for table '%s' "
                "because it's of type %s\n", table, table_type);
  }

  complete_insert= 0;
  if ((write_data= !(*ignore_flag & IGNORE_DATA)))
  {
    complete_insert= opt_complete_insert;
    if (!insert_pat_inited)
    {
      insert_pat_inited= 1;
      init_dynamic_string_checked(&insert_pat, "", 1024, 1024);
    }
    else
      dynstr_set_checked(&insert_pat, "");
  }

  insert_option= ((delayed && opt_ignore) ? " DELAYED IGNORE " :
                  delayed ? " DELAYED " : opt_ignore ? " IGNORE " : "");

  verbose_msg("-- Retrieving table structure for table %s...\n", table);

  len= my_snprintf(query_buff, sizeof(query_buff),
                   "SET SQL_QUOTE_SHOW_CREATE=%d",
                   (opt_quoted || opt_keywords));
  if (!create_options)
    strmov(query_buff+len,
           "/*!40102 ,SQL_MODE=concat(@@sql_mode, _utf8 ',NO_KEY_OPTIONS,NO_TABLE_OPTIONS,NO_FIELD_OPTIONS') */");

  result_table=     quote_name(table, table_buff, 1);
  opt_quoted_table= quote_name(table, table_buff2, 0);

  if (opt_order_by_primary)
    order_by= primary_key_fields(result_table);

  if (!opt_xml && !mysql_query_with_error_report(mysql, 0, query_buff))
  {
    /* using SHOW CREATE statement */
    if (!opt_no_create_info)
    {
      /* Make an sql-file, if path was given iow. option -T was given */
      char buff[20+FN_REFLEN];
      MYSQL_FIELD *field;

      my_snprintf(buff, sizeof(buff), "show create table %s", result_table);

      if (switch_character_set_results(mysql, "binary") ||
          mysql_query_with_error_report(mysql, &result, buff) ||
          switch_character_set_results(mysql, default_charset))
        DBUG_RETURN(0);

      if (path)
      {
        if (!(sql_file= open_sql_file_for_table(table, O_WRONLY)))
          DBUG_RETURN(0);

        write_header(sql_file, db);
      }

      if (strcmp (table_type, "VIEW") == 0)         /* view */
        print_comment(sql_file, 0,
                      "\n--\n-- Temporary table structure for view %s\n--\n\n",
                      result_table);
      else
        print_comment(sql_file, 0,
                      "\n--\n-- Table structure for table %s\n--\n\n",
                      result_table);

      if (opt_drop)
      {
      /*
        Even if the "table" is a view, we do a DROP TABLE here.  The
        view-specific code below fills in the DROP VIEW.
        We will skip the DROP TABLE for general_log and slow_log, since
        those stmts will fail, in case we apply dump by enabling logging.
       */
        if (!general_log_or_slow_log_tables(db, table))
          fprintf(sql_file, "DROP TABLE IF EXISTS %s;\n",
                  opt_quoted_table);
        check_io(sql_file);
      }

      field= mysql_fetch_field_direct(result, 0);
      if (strcmp(field->name, "View") == 0)
      {
        char *scv_buff= NULL;
        my_ulonglong n_cols;

        verbose_msg("-- It's a view, create dummy table for view\n");

        /* save "show create" statement for later */
        if ((row= mysql_fetch_row(result)) && (scv_buff=row[1]))
          scv_buff= my_strdup(scv_buff, MYF(0));

        mysql_free_result(result);

        /*
          Create a table with the same name as the view and with columns of
          the same name in order to satisfy views that depend on this view.
          The table will be removed when the actual view is created.

          The properties of each column, are not preserved in this temporary
          table, because they are not necessary.

          This will not be necessary once we can determine dependencies
          between views and can simply dump them in the appropriate order.
        */
        my_snprintf(query_buff, sizeof(query_buff),
                    "SHOW FIELDS FROM %s", result_table);
        if (switch_character_set_results(mysql, "binary") ||
            mysql_query_with_error_report(mysql, &result, query_buff) ||
            switch_character_set_results(mysql, default_charset))
        {
          /*
            View references invalid or privileged table/col/fun (err 1356),
            so we cannot create a stand-in table.  Be defensive and dump
            a comment with the view's 'show create' statement. (Bug #17371)
          */

          if (mysql_errno(mysql) == ER_VIEW_INVALID)
            fprintf(sql_file, "\n-- failed on view %s: %s\n\n", result_table, scv_buff ? scv_buff : "");

          my_free(scv_buff);

          DBUG_RETURN(0);
        }
        else
          my_free(scv_buff);

        n_cols= mysql_num_rows(result);
        if (0 != n_cols)
        {

          /*
            The actual formula is based on the column names and how the .FRM
            files are stored and is too volatile to be repeated here.
            Thus we simply warn the user if the columns exceed a limit we
            know works most of the time.
          */
          if (n_cols >= 1000)
            fprintf(stderr,
                    "-- Warning: Creating a stand-in table for view %s may"
                    " fail when replaying the dump file produced because "
                    "of the number of columns exceeding 1000. Exercise "
                    "caution when replaying the produced dump file.\n", 
                    table);
          if (opt_drop)
          {
            /*
              We have already dropped any table of the same name above, so
              here we just drop the view.
            */

            fprintf(sql_file, "/*!50001 DROP VIEW IF EXISTS %s*/;\n",
                    opt_quoted_table);
            check_io(sql_file);
          }

          fprintf(sql_file,
                  "SET @saved_cs_client     = @@character_set_client;\n"
                  "SET character_set_client = utf8;\n"
                  "/*!50001 CREATE TABLE %s (\n",
                  result_table);

          /*
            Get first row, following loop will prepend comma - keeps from
            having to know if the row being printed is last to determine if
            there should be a _trailing_ comma.
          */

          row= mysql_fetch_row(result);

          /*
            The actual column type doesn't matter anyway, since the table will
            be dropped at run time.
            We do tinyint to avoid hitting the row size limit.
          */
          fprintf(sql_file, "  %s tinyint NOT NULL",
                  quote_name(row[0], name_buff, 0));

          while((row= mysql_fetch_row(result)))
          {
            /* col name, col type */
            fprintf(sql_file, ",\n  %s tinyint NOT NULL",
                    quote_name(row[0], name_buff, 0));
          }

          /*
            Stand-in tables are always MyISAM tables as the default
            engine might have a column-limit that's lower than the
            number of columns in the view, and MyISAM support is
            guaranteed to be in the server anyway.
          */
          fprintf(sql_file,
                  "\n) ENGINE=MyISAM */;\n"
                  "SET character_set_client = @saved_cs_client;\n");

          check_io(sql_file);
        }

        mysql_free_result(result);

        if (path)
          my_fclose(sql_file, MYF(MY_WME));

        seen_views= 1;
        DBUG_RETURN(0);
      }

      row= mysql_fetch_row(result);

      is_log_table= general_log_or_slow_log_tables(db, table);
      if (is_log_table)
        row[1]+= 13; /* strlen("CREATE TABLE ")= 13 */
      if (opt_compatible_mode & 3)
      {
        fprintf(sql_file,
                is_log_table ? "CREATE TABLE IF NOT EXISTS %s;\n" : "%s;\n",
                row[1]);
      }
      else
      {
        fprintf(sql_file,
                "/*!40101 SET @saved_cs_client     = @@character_set_client */;\n"
                "/*!40101 SET character_set_client = utf8 */;\n"
                "%s%s;\n"
                "/*!40101 SET character_set_client = @saved_cs_client */;\n",
                is_log_table ? "CREATE TABLE IF NOT EXISTS " : "",
                row[1]);
      }

      check_io(sql_file);
      mysql_free_result(result);
    }
    my_snprintf(query_buff, sizeof(query_buff), "show fields from %s",
                result_table);
    if (mysql_query_with_error_report(mysql, &result, query_buff))
    {
      if (path)
        my_fclose(sql_file, MYF(MY_WME));
      DBUG_RETURN(0);
    }

    /*
      If write_data is true, then we build up insert statements for
      the table's data. Note: in subsequent lines of code, this test
      will have to be performed each time we are appending to
      insert_pat.
    */
    if (write_data)
    {
      if (opt_replace_into)
        dynstr_append_checked(&insert_pat, "REPLACE ");
      else
        dynstr_append_checked(&insert_pat, "INSERT ");
      dynstr_append_checked(&insert_pat, insert_option);
      dynstr_append_checked(&insert_pat, "INTO ");
      dynstr_append_checked(&insert_pat, opt_quoted_table);
      if (complete_insert)
      {
        dynstr_append_checked(&insert_pat, " (");
      }
      else
      {
        dynstr_append_checked(&insert_pat, " VALUES ");
        if (!extended_insert)
          dynstr_append_checked(&insert_pat, "(");
      }
    }

    while ((row= mysql_fetch_row(result)))
    {
      if (complete_insert)
      {
        if (init)
        {
          dynstr_append_checked(&insert_pat, ", ");
        }
        init=1;
        dynstr_append_checked(&insert_pat,
                      quote_name(row[SHOW_FIELDNAME], name_buff, 0));
      }
    }
    num_fields= mysql_num_rows(result);
    mysql_free_result(result);
  }
  else
  {
    verbose_msg("%s: Warning: Can't set SQL_QUOTE_SHOW_CREATE option (%s)\n",
                my_progname_short, mysql_error(mysql));

    my_snprintf(query_buff, sizeof(query_buff), show_fields_stmt, db, table);

    if (mysql_query_with_error_report(mysql, &result, query_buff))
      DBUG_RETURN(0);

    /* Make an sql-file, if path was given iow. option -T was given */
    if (!opt_no_create_info)
    {
      if (path)
      {
        if (!(sql_file= open_sql_file_for_table(table, O_WRONLY)))
          DBUG_RETURN(0);
        write_header(sql_file, db);
      }

      print_comment(sql_file, 0,
                    "\n--\n-- Table structure for table %s\n--\n\n",
                    result_table);
      if (opt_drop)
        fprintf(sql_file, "DROP TABLE IF EXISTS %s;\n", result_table);
      if (!opt_xml)
        fprintf(sql_file, "CREATE TABLE %s (\n", result_table);
      else
        print_xml_tag(sql_file, "\t", "\n", "table_structure", "name=", table, 
                NullS);
      check_io(sql_file);
    }

    if (write_data)
    {
      if (opt_replace_into)
        dynstr_append_checked(&insert_pat, "REPLACE ");
      else
        dynstr_append_checked(&insert_pat, "INSERT ");
      dynstr_append_checked(&insert_pat, insert_option);
      dynstr_append_checked(&insert_pat, "INTO ");
      dynstr_append_checked(&insert_pat, result_table);
      if (complete_insert)
        dynstr_append_checked(&insert_pat, " (");
      else
      {
        dynstr_append_checked(&insert_pat, " VALUES ");
        if (!extended_insert)
          dynstr_append_checked(&insert_pat, "(");
      }
    }

    while ((row= mysql_fetch_row(result)))
    {
      ulong *lengths= mysql_fetch_lengths(result);
      if (init)
      {
        if (!opt_xml && !opt_no_create_info)
        {
          fputs(",\n",sql_file);
          check_io(sql_file);
        }
        if (complete_insert)
          dynstr_append_checked(&insert_pat, ", ");
      }
      init=1;
      if (complete_insert)
        dynstr_append_checked(&insert_pat,
                      quote_name(row[SHOW_FIELDNAME], name_buff, 0));
      if (!opt_no_create_info)
      {
        if (opt_xml)
        {
          print_xml_row(sql_file, "field", result, &row, NullS);
          continue;
        }

        if (opt_keywords)
          fprintf(sql_file, "  %s.%s %s", result_table,
                  quote_name(row[SHOW_FIELDNAME],name_buff, 0),
                  row[SHOW_TYPE]);
        else
          fprintf(sql_file, "  %s %s", quote_name(row[SHOW_FIELDNAME],
                                                  name_buff, 0),
                  row[SHOW_TYPE]);
        if (row[SHOW_DEFAULT])
        {
          fputs(" DEFAULT ", sql_file);
          unescape(sql_file, row[SHOW_DEFAULT], lengths[SHOW_DEFAULT]);
        }
        if (!row[SHOW_NULL][0])
          fputs(" NOT NULL", sql_file);
        if (row[SHOW_EXTRA][0])
          fprintf(sql_file, " %s",row[SHOW_EXTRA]);
        check_io(sql_file);
      }
    }
    num_fields= mysql_num_rows(result);
    mysql_free_result(result);
    if (!opt_no_create_info)
    {
      /* Make an sql-file, if path was given iow. option -T was given */
      char buff[20+FN_REFLEN];
      uint keynr,primary_key;
      my_snprintf(buff, sizeof(buff), "show keys from %s", result_table);
      if (mysql_query_with_error_report(mysql, &result, buff))
      {
        if (mysql_errno(mysql) == ER_WRONG_OBJECT)
        {
          /* it is VIEW */
          fputs("\t\t<options Comment=\"view\" />\n", sql_file);
          goto continue_xml;
        }
        fprintf(stderr, "%s: Can't get keys for table %s (%s)\n",
                my_progname_short, result_table, mysql_error(mysql));
        if (path)
          my_fclose(sql_file, MYF(MY_WME));
        DBUG_RETURN(0);
      }

      /* Find first which key is primary key */
      keynr=0;
      primary_key=INT_MAX;
      while ((row= mysql_fetch_row(result)))
      {
        if (atoi(row[3]) == 1)
        {
          keynr++;
#ifdef FORCE_PRIMARY_KEY
          if (atoi(row[1]) == 0 && primary_key == INT_MAX)
            primary_key=keynr;
#endif
          if (!strcmp(row[2],"PRIMARY"))
          {
            primary_key=keynr;
            break;
          }
        }
      }
      mysql_data_seek(result,0);
      keynr=0;
      while ((row= mysql_fetch_row(result)))
      {
        if (opt_xml)
        {
          print_xml_row(sql_file, "key", result, &row, NullS);
          continue;
        }

        if (atoi(row[3]) == 1)
        {
          if (keynr++)
            putc(')', sql_file);
          if (atoi(row[1]))       /* Test if duplicate key */
            /* Duplicate allowed */
            fprintf(sql_file, ",\n  KEY %s (",quote_name(row[2],name_buff,0));
          else if (keynr == primary_key)
            fputs(",\n  PRIMARY KEY (",sql_file); /* First UNIQUE is primary */
          else
            fprintf(sql_file, ",\n  UNIQUE %s (",quote_name(row[2],name_buff,
                                                            0));
        }
        else
          putc(',', sql_file);
        fputs(quote_name(row[4], name_buff, 0), sql_file);
        if (row[7])
          fprintf(sql_file, " (%s)",row[7]);      /* Sub key */
        check_io(sql_file);
      }
      mysql_free_result(result);
      if (!opt_xml)
      {
        if (keynr)
          putc(')', sql_file);
        fputs("\n)",sql_file);
        check_io(sql_file);
      }

      /* Get MySQL specific create options */
      if (create_options)
      {
        char show_name_buff[NAME_LEN*2+2+24];

        /* Check memory for quote_for_like() */
        my_snprintf(buff, sizeof(buff), "show table status like %s",
                    quote_for_like(table, show_name_buff));

        if (mysql_query_with_error_report(mysql, &result, buff))
        {
          if (mysql_errno(mysql) != ER_PARSE_ERROR)
          {                                     /* If old MySQL version */
            verbose_msg("-- Warning: Couldn't get status information for " \
                        "table %s (%s)\n", result_table,mysql_error(mysql));
          }
        }
        else if (!(row= mysql_fetch_row(result)))
        {
          fprintf(stderr,
                  "Error: Couldn't read status information for table %s (%s)\n",
                  result_table,mysql_error(mysql));
        }
        else
        {
          if (opt_xml)
            print_xml_row(sql_file, "options", result, &row, NullS);
          else
          {
            fputs("/*!",sql_file);
            print_value(sql_file,result,row,"engine=","Engine",0);
            print_value(sql_file,result,row,"","Create_options",0);
            print_value(sql_file,result,row,"comment=","Comment",1);
            fputs(" */",sql_file);
            check_io(sql_file);
          }
        }
        mysql_free_result(result);              /* Is always safe to free */
      }
continue_xml:
      if (!opt_xml)
        fputs(";\n", sql_file);
      else
        fputs("\t</table_structure>\n", sql_file);
      check_io(sql_file);
    }
  }
  if (complete_insert)
  {
    dynstr_append_checked(&insert_pat, ") VALUES ");
    if (!extended_insert)
      dynstr_append_checked(&insert_pat, "(");
  }
  if (sql_file != md_result_file)
  {
    fputs("\n", sql_file);
    write_footer(sql_file);
    my_fclose(sql_file, MYF(MY_WME));
  }
  DBUG_RETURN((uint) num_fields);
} /* get_table_structure */