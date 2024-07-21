static my_bool get_view_structure(char *table, char* db)
{
  MYSQL_RES  *table_res;
  MYSQL_ROW  row;
  MYSQL_FIELD *field;
  char       *result_table, *opt_quoted_table;
  char       table_buff[NAME_LEN*2+3];
  char       table_buff2[NAME_LEN*2+3];
  char       query[QUERY_LENGTH];
  FILE       *sql_file= md_result_file;
  DBUG_ENTER("get_view_structure");

  if (opt_no_create_info) /* Don't write table creation info */
    DBUG_RETURN(0);

  verbose_msg("-- Retrieving view structure for table %s...\n", table);

#ifdef NOT_REALLY_USED_YET
  dynstr_append_checked(&insert_pat, "SET SQL_QUOTE_SHOW_CREATE=");
  dynstr_append_checked(&insert_pat, (opt_quoted || opt_keywords)? "1":"0");
#endif

  result_table=     quote_name(table, table_buff, 1);
  opt_quoted_table= quote_name(table, table_buff2, 0);

  if (switch_character_set_results(mysql, "binary"))
    DBUG_RETURN(1);

  my_snprintf(query, sizeof(query), "SHOW CREATE TABLE %s", result_table);

  if (mysql_query_with_error_report(mysql, &table_res, query))
  {
    switch_character_set_results(mysql, default_charset);
    DBUG_RETURN(0);
  }

  /* Check if this is a view */
  field= mysql_fetch_field_direct(table_res, 0);
  if (strcmp(field->name, "View") != 0)
  {
    switch_character_set_results(mysql, default_charset);
    verbose_msg("-- It's base table, skipped\n");
    DBUG_RETURN(0);
  }

  /* If requested, open separate .sql file for this view */
  if (path)
  {
    if (!(sql_file= open_sql_file_for_table(table, O_WRONLY)))
      DBUG_RETURN(1);

    write_header(sql_file, db);
  }

  print_comment(sql_file, 0,
                "\n--\n-- Final view structure for view %s\n--\n\n",
                fix_identifier_with_newline(result_table));

  /* Table might not exist if this view was dumped with --tab. */
  fprintf(sql_file, "/*!50001 DROP TABLE IF EXISTS %s*/;\n", opt_quoted_table);
  if (opt_drop)
  {
    fprintf(sql_file, "/*!50001 DROP VIEW IF EXISTS %s*/;\n",
            opt_quoted_table);
    check_io(sql_file);
  }


  my_snprintf(query, sizeof(query),
              "SELECT CHECK_OPTION, DEFINER, SECURITY_TYPE, "
              "       CHARACTER_SET_CLIENT, COLLATION_CONNECTION "
              "FROM information_schema.views "
              "WHERE table_name=\"%s\" AND table_schema=\"%s\"", table, db);

  if (mysql_query(mysql, query))
  {
    /*
      Use the raw output from SHOW CREATE TABLE if
       information_schema query fails.
     */
    row= mysql_fetch_row(table_res);
    fprintf(sql_file, "/*!50001 %s */;\n", row[1]);
    check_io(sql_file);
    mysql_free_result(table_res);
  }
  else
  {
    char *ptr;
    ulong *lengths;
    char search_buf[256], replace_buf[256];
    ulong search_len, replace_len;
    DYNAMIC_STRING ds_view;

    /* Save the result of SHOW CREATE TABLE in ds_view */
    row= mysql_fetch_row(table_res);
    lengths= mysql_fetch_lengths(table_res);
    init_dynamic_string_checked(&ds_view, row[1], lengths[1] + 1, 1024);
    mysql_free_result(table_res);

    /* Get the result from "select ... information_schema" */
    if (!(table_res= mysql_store_result(mysql)) ||
        !(row= mysql_fetch_row(table_res)))
    {
      if (table_res)
        mysql_free_result(table_res);
      dynstr_free(&ds_view);
      DB_error(mysql, "when trying to save the result of SHOW CREATE TABLE in ds_view.");
      DBUG_RETURN(1);
    }

    lengths= mysql_fetch_lengths(table_res);

    /*
      "WITH %s CHECK OPTION" is available from 5.0.2
      Surround it with !50002 comments
    */
    if (strcmp(row[0], "NONE"))
    {

      ptr= search_buf;
      search_len= (ulong)(strxmov(ptr, "WITH ", row[0],
                                  " CHECK OPTION", NullS) - ptr);
      ptr= replace_buf;
      replace_len=(ulong)(strxmov(ptr, "*/\n/*!50002 WITH ", row[0],
                                  " CHECK OPTION", NullS) - ptr);
      replace(&ds_view, search_buf, search_len, replace_buf, replace_len);
    }

    /*
      "DEFINER=%s SQL SECURITY %s" is available from 5.0.13
      Surround it with !50013 comments
    */
    {
      size_t     user_name_len;
      char       user_name_str[USERNAME_LENGTH + 1];
      char       quoted_user_name_str[USERNAME_LENGTH * 2 + 3];
      size_t     host_name_len;
      char       host_name_str[HOSTNAME_LENGTH + 1];
      char       quoted_host_name_str[HOSTNAME_LENGTH * 2 + 3];

      parse_user(row[1], lengths[1], user_name_str, &user_name_len,
                 host_name_str, &host_name_len);

      ptr= search_buf;
      search_len=
        (ulong)(strxmov(ptr, "DEFINER=",
                        quote_name(user_name_str, quoted_user_name_str, FALSE),
                        "@",
                        quote_name(host_name_str, quoted_host_name_str, FALSE),
                        " SQL SECURITY ", row[2], NullS) - ptr);
      ptr= replace_buf;
      replace_len=
        (ulong)(strxmov(ptr, "*/\n/*!50013 DEFINER=",
                        quote_name(user_name_str, quoted_user_name_str, FALSE),
                        "@",
                        quote_name(host_name_str, quoted_host_name_str, FALSE),
                        " SQL SECURITY ", row[2],
                        " */\n/*!50001", NullS) - ptr);
      replace(&ds_view, search_buf, search_len, replace_buf, replace_len);
    }

    /* Dump view structure to file */

    fprintf(sql_file,
            "/*!50001 SET @saved_cs_client          = @@character_set_client */;\n"
            "/*!50001 SET @saved_cs_results         = @@character_set_results */;\n"
            "/*!50001 SET @saved_col_connection     = @@collation_connection */;\n"
            "/*!50001 SET character_set_client      = %s */;\n"
            "/*!50001 SET character_set_results     = %s */;\n"
            "/*!50001 SET collation_connection      = %s */;\n"
            "/*!50001 %s */;\n"
            "/*!50001 SET character_set_client      = @saved_cs_client */;\n"
            "/*!50001 SET character_set_results     = @saved_cs_results */;\n"
            "/*!50001 SET collation_connection      = @saved_col_connection */;\n",
            (const char *) row[3],
            (const char *) row[3],
            (const char *) row[4],
            (const char *) ds_view.str);

    check_io(sql_file);
    mysql_free_result(table_res);
    dynstr_free(&ds_view);
  }

  if (switch_character_set_results(mysql, default_charset))
    DBUG_RETURN(1);

  /* If a separate .sql file was opened, close it now */
  if (sql_file != md_result_file)
  {
    fputs("\n", sql_file);
    write_footer(sql_file);
    my_fclose(sql_file, MYF(MY_WME));
  }
  DBUG_RETURN(0);
}