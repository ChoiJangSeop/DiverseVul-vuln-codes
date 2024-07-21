static uint dump_events_for_db(char *db)
{
  char       query_buff[QUERY_LENGTH];
  char       db_name_buff[NAME_LEN*2+3], name_buff[NAME_LEN*2+3];
  char       *event_name;
  char       delimiter[QUERY_LENGTH];
  FILE       *sql_file= md_result_file;
  MYSQL_RES  *event_res, *event_list_res;
  MYSQL_ROW  row, event_list_row;

  char       db_cl_name[MY_CS_NAME_SIZE];
  int        db_cl_altered= FALSE;

  DBUG_ENTER("dump_events_for_db");
  DBUG_PRINT("enter", ("db: '%s'", db));

  mysql_real_escape_string(mysql, db_name_buff, db, (ulong)strlen(db));

  /* nice comments */
  print_comment(sql_file, 0,
                "\n--\n-- Dumping events for database '%s'\n--\n",
                fix_identifier_with_newline(db));

  /*
    not using "mysql_query_with_error_report" because we may have not
    enough privileges to lock mysql.events.
  */
  if (lock_tables)
    mysql_query(mysql, "LOCK TABLES mysql.event READ");

  if (mysql_query_with_error_report(mysql, &event_list_res, "show events"))
    DBUG_RETURN(0);

  strcpy(delimiter, ";");
  if (mysql_num_rows(event_list_res) > 0)
  {
    if (opt_xml)
      fputs("\t<events>\n", sql_file);
    else
    {
      fprintf(sql_file, "/*!50106 SET @save_time_zone= @@TIME_ZONE */ ;\n");

      /* Get database collation. */

      if (fetch_db_collation(db_name_buff, db_cl_name, sizeof (db_cl_name)))
        DBUG_RETURN(1);
    }

    if (switch_character_set_results(mysql, "binary"))
      DBUG_RETURN(1);

    while ((event_list_row= mysql_fetch_row(event_list_res)) != NULL)
    {
      event_name= quote_name(event_list_row[1], name_buff, 0);
      DBUG_PRINT("info", ("retrieving CREATE EVENT for %s", name_buff));
      my_snprintf(query_buff, sizeof(query_buff), "SHOW CREATE EVENT %s", 
          event_name);

      if (mysql_query_with_error_report(mysql, &event_res, query_buff))
        DBUG_RETURN(1);

      while ((row= mysql_fetch_row(event_res)) != NULL)
      {
        if (opt_xml)
        {
          print_xml_row(sql_file, "event", event_res, &row,
                        "Create Event");
          continue;
        }

        /*
          if the user has EXECUTE privilege he can see event names, but not the
          event body!
        */
        if (strlen(row[3]) != 0)
        {
          char *query_str;

          if (opt_drop)
            fprintf(sql_file, "/*!50106 DROP EVENT IF EXISTS %s */%s\n", 
                event_name, delimiter);

          if (create_delimiter(row[3], delimiter, sizeof(delimiter)) == NULL)
          {
            fprintf(stderr, "%s: Warning: Can't create delimiter for event '%s'\n",
                    my_progname, event_name);
            DBUG_RETURN(1);
          }

          fprintf(sql_file, "DELIMITER %s\n", delimiter);

          if (mysql_num_fields(event_res) >= 7)
          {
            if (switch_db_collation(sql_file, db_name_buff, delimiter,
                                    db_cl_name, row[6], &db_cl_altered))
            {
              DBUG_RETURN(1);
            }

            switch_cs_variables(sql_file, delimiter,
                                row[4],   /* character_set_client */
                                row[4],   /* character_set_results */
                                row[5]);  /* collation_connection */
          }
          else
          {
            /*
              mysqldump is being run against the server, that does not
              provide character set information in SHOW CREATE
              statements.

              NOTE: the dump may be incorrect, since character set
              information is required in order to restore event properly.
            */

            fprintf(sql_file,
                    "--\n"
                    "-- WARNING: old server version. "
                      "The following dump may be incomplete.\n"
                    "--\n");
          }

          switch_sql_mode(sql_file, delimiter, row[1]);

          switch_time_zone(sql_file, delimiter, row[2]);

          query_str= cover_definer_clause(row[3], strlen(row[3]),
                                          C_STRING_WITH_LEN("50117"),
                                          C_STRING_WITH_LEN("50106"),
                                          C_STRING_WITH_LEN(" EVENT"));

          fprintf(sql_file,
                  "/*!50106 %s */ %s\n",
                  (const char *) (query_str != NULL ? query_str : row[3]),
                  (const char *) delimiter);

          if(query_str)
          {
            my_free(query_str);
            query_str= NULL;
          }
          restore_time_zone(sql_file, delimiter);
          restore_sql_mode(sql_file, delimiter);

          if (mysql_num_fields(event_res) >= 7)
          {
            restore_cs_variables(sql_file, delimiter);

            if (db_cl_altered)
            {
              if (restore_db_collation(sql_file, db_name_buff, delimiter,
                                       db_cl_name))
                DBUG_RETURN(1);
            }
          }
        }
      } /* end of event printing */
      mysql_free_result(event_res);

    } /* end of list of events */
    if (opt_xml)
    {
      fputs("\t</events>\n", sql_file);
      check_io(sql_file);
    }
    else
    {
      fprintf(sql_file, "DELIMITER ;\n");
      fprintf(sql_file, "/*!50106 SET TIME_ZONE= @save_time_zone */ ;\n");
    }

    if (switch_character_set_results(mysql, default_charset))
      DBUG_RETURN(1);
  }
  mysql_free_result(event_list_res);

  if (lock_tables)
    (void) mysql_query_with_error_report(mysql, 0, "UNLOCK TABLES");
  DBUG_RETURN(0);
}