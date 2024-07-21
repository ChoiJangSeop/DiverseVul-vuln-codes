get_options(int *argc,char ***argv)
{
  int ho_error;
  char *tmp_string;
  MY_STAT sbuf;  /* Stat information for the data file */

  DBUG_ENTER("get_options");
  if ((ho_error= handle_options(argc, argv, my_long_options, get_one_option)))
    exit(ho_error);
  if (debug_info_flag)
    my_end_arg= MY_CHECK_ERROR | MY_GIVE_INFO;
  if (debug_check_flag)
    my_end_arg= MY_CHECK_ERROR;

  if (!user)
    user= (char *)"root";

  /*
    If something is created and --no-drop is not specified, we drop the
    schema.
  */
  if (!opt_no_drop && (create_string || auto_generate_sql))
    opt_preserve= FALSE;

  if (auto_generate_sql && (create_string || user_supplied_query))
  {
      fprintf(stderr,
              "%s: Can't use --auto-generate-sql when create and query strings are specified!\n",
              my_progname);
      exit(1);
  }

  if (auto_generate_sql && auto_generate_sql_guid_primary && 
      auto_generate_sql_autoincrement)
  {
      fprintf(stderr,
              "%s: Either auto-generate-sql-guid-primary or auto-generate-sql-add-autoincrement can be used!\n",
              my_progname);
      exit(1);
  }

  /* 
    We are testing to make sure that if someone specified a key search
    that we actually added a key!
  */
  if (auto_generate_sql && auto_generate_sql_type[0] == 'k')
    if ( auto_generate_sql_autoincrement == FALSE &&
         auto_generate_sql_guid_primary == FALSE)
    {
      fprintf(stderr,
              "%s: Can't perform key test without a primary key!\n",
              my_progname);
      exit(1);
    }



  if (auto_generate_sql && num_of_query && auto_actual_queries)
  {
      fprintf(stderr,
              "%s: Either auto-generate-sql-execute-number or number-of-queries can be used!\n",
              my_progname);
      exit(1);
  }

  parse_comma(concurrency_str ? concurrency_str : "1", &concurrency);

  if (opt_csv_str)
  {
    opt_silent= TRUE;
    
    if (opt_csv_str[0] == '-')
    {
      csv_file= fileno(stdout);
    }
    else
    {
      if ((csv_file= my_open(opt_csv_str, O_CREAT|O_WRONLY|O_APPEND, MYF(0)))
          == -1)
      {
        fprintf(stderr,"%s: Could not open csv file: %sn\n",
                my_progname, opt_csv_str);
        exit(1);
      }
    }
  }

  if (opt_only_print)
    opt_silent= TRUE;

  if (num_int_cols_opt)
  {
    option_string *str;
    parse_option(num_int_cols_opt, &str, ',');
    num_int_cols= atoi(str->string);
    if (str->option)
      num_int_cols_index= atoi(str->option);
    option_cleanup(str);
  }

  if (num_char_cols_opt)
  {
    option_string *str;
    parse_option(num_char_cols_opt, &str, ',');
    num_char_cols= atoi(str->string);
    if (str->option)
      num_char_cols_index= atoi(str->option);
    else
      num_char_cols_index= 0;
    option_cleanup(str);
  }


  if (auto_generate_sql)
  {
    unsigned long long x= 0;
    statement *ptr_statement;

    if (verbose >= 2)
      printf("Building Create Statements for Auto\n");

    create_statements= build_table_string();
    /* 
      Pre-populate table 
    */
    for (ptr_statement= create_statements, x= 0; 
         x < auto_generate_sql_unique_write_number; 
         x++, ptr_statement= ptr_statement->next)
    {
      ptr_statement->next= build_insert_string();
    }

    if (verbose >= 2)
      printf("Building Query Statements for Auto\n");

    if (auto_generate_sql_type[0] == 'r')
    {
      if (verbose >= 2)
        printf("Generating SELECT Statements for Auto\n");

      query_statements= build_select_string(FALSE);
      for (ptr_statement= query_statements, x= 0; 
           x < auto_generate_sql_unique_query_number; 
           x++, ptr_statement= ptr_statement->next)
      {
        ptr_statement->next= build_select_string(FALSE);
      }
    }
    else if (auto_generate_sql_type[0] == 'k')
    {
      if (verbose >= 2)
        printf("Generating SELECT for keys Statements for Auto\n");

      query_statements= build_select_string(TRUE);
      for (ptr_statement= query_statements, x= 0; 
           x < auto_generate_sql_unique_query_number; 
           x++, ptr_statement= ptr_statement->next)
      {
        ptr_statement->next= build_select_string(TRUE);
      }
    }
    else if (auto_generate_sql_type[0] == 'w')
    {
      /*
        We generate a number of strings in case the engine is 
        Archive (since strings which were identical one after another
        would be too easily optimized).
      */
      if (verbose >= 2)
        printf("Generating INSERT Statements for Auto\n");
      query_statements= build_insert_string();
      for (ptr_statement= query_statements, x= 0; 
           x < auto_generate_sql_unique_query_number; 
           x++, ptr_statement= ptr_statement->next)
      {
        ptr_statement->next= build_insert_string();
      }
    }
    else if (auto_generate_sql_type[0] == 'u')
    {
      query_statements= build_update_string();
      for (ptr_statement= query_statements, x= 0; 
           x < auto_generate_sql_unique_query_number; 
           x++, ptr_statement= ptr_statement->next)
      {
          ptr_statement->next= build_update_string();
      }
    }
    else /* Mixed mode is default */
    {
      int coin= 0;

      query_statements= build_insert_string();
      /* 
        This logic should be extended to do a more mixed load,
        at the moment it results in "every other".
      */
      for (ptr_statement= query_statements, x= 0; 
           x < auto_generate_sql_unique_query_number; 
           x++, ptr_statement= ptr_statement->next)
      {
        if (coin)
        {
          ptr_statement->next= build_insert_string();
          coin= 0;
        }
        else
        {
          ptr_statement->next= build_select_string(TRUE);
          coin= 1;
        }
      }
    }
  }
  else
  {
    if (create_string && my_stat(create_string, &sbuf, MYF(0)))
    {
      File data_file;
      if (!MY_S_ISREG(sbuf.st_mode))
      {
        fprintf(stderr,"%s: Create file was not a regular file\n",
                my_progname);
        exit(1);
      }
      if ((data_file= my_open(create_string, O_RDWR, MYF(0))) == -1)
      {
        fprintf(stderr,"%s: Could not open create file\n", my_progname);
        exit(1);
      }
      tmp_string= (char *)my_malloc(sbuf.st_size + 1,
                              MYF(MY_ZEROFILL|MY_FAE|MY_WME));
      my_read(data_file, (uchar*) tmp_string, sbuf.st_size, MYF(0));
      tmp_string[sbuf.st_size]= '\0';
      my_close(data_file,MYF(0));
      parse_delimiter(tmp_string, &create_statements, delimiter[0]);
      my_free(tmp_string, MYF(0));
    }
    else if (create_string)
    {
        parse_delimiter(create_string, &create_statements, delimiter[0]);
    }

    if (user_supplied_query && my_stat(user_supplied_query, &sbuf, MYF(0)))
    {
      File data_file;
      if (!MY_S_ISREG(sbuf.st_mode))
      {
        fprintf(stderr,"%s: User query supplied file was not a regular file\n",
                my_progname);
        exit(1);
      }
      if ((data_file= my_open(user_supplied_query, O_RDWR, MYF(0))) == -1)
      {
        fprintf(stderr,"%s: Could not open query supplied file\n", my_progname);
        exit(1);
      }
      tmp_string= (char *)my_malloc(sbuf.st_size + 1,
                                    MYF(MY_ZEROFILL|MY_FAE|MY_WME));
      my_read(data_file, (uchar*) tmp_string, sbuf.st_size, MYF(0));
      tmp_string[sbuf.st_size]= '\0';
      my_close(data_file,MYF(0));
      if (user_supplied_query)
        actual_queries= parse_delimiter(tmp_string, &query_statements,
                                        delimiter[0]);
      my_free(tmp_string, MYF(0));
    } 
    else if (user_supplied_query)
    {
        actual_queries= parse_delimiter(user_supplied_query, &query_statements,
                                        delimiter[0]);
    }
  }

  if (user_supplied_pre_statements && my_stat(user_supplied_pre_statements, &sbuf, MYF(0)))
  {
    File data_file;
    if (!MY_S_ISREG(sbuf.st_mode))
    {
      fprintf(stderr,"%s: User query supplied file was not a regular file\n",
              my_progname);
      exit(1);
    }
    if ((data_file= my_open(user_supplied_pre_statements, O_RDWR, MYF(0))) == -1)
    {
      fprintf(stderr,"%s: Could not open query supplied file\n", my_progname);
      exit(1);
    }
    tmp_string= (char *)my_malloc(sbuf.st_size + 1,
                                  MYF(MY_ZEROFILL|MY_FAE|MY_WME));
    my_read(data_file, (uchar*) tmp_string, sbuf.st_size, MYF(0));
    tmp_string[sbuf.st_size]= '\0';
    my_close(data_file,MYF(0));
    if (user_supplied_pre_statements)
      (void)parse_delimiter(tmp_string, &pre_statements,
                            delimiter[0]);
    my_free(tmp_string, MYF(0));
  } 
  else if (user_supplied_pre_statements)
  {
    (void)parse_delimiter(user_supplied_pre_statements,
                          &pre_statements,
                          delimiter[0]);
  }

  if (user_supplied_post_statements && my_stat(user_supplied_post_statements, &sbuf, MYF(0)))
  {
    File data_file;
    if (!MY_S_ISREG(sbuf.st_mode))
    {
      fprintf(stderr,"%s: User query supplied file was not a regular file\n",
              my_progname);
      exit(1);
    }
    if ((data_file= my_open(user_supplied_post_statements, O_RDWR, MYF(0))) == -1)
    {
      fprintf(stderr,"%s: Could not open query supplied file\n", my_progname);
      exit(1);
    }
    tmp_string= (char *)my_malloc(sbuf.st_size + 1,
                                  MYF(MY_ZEROFILL|MY_FAE|MY_WME));
    my_read(data_file, (uchar*) tmp_string, sbuf.st_size, MYF(0));
    tmp_string[sbuf.st_size]= '\0';
    my_close(data_file,MYF(0));
    if (user_supplied_post_statements)
      (void)parse_delimiter(tmp_string, &post_statements,
                            delimiter[0]);
    my_free(tmp_string, MYF(0));
  } 
  else if (user_supplied_post_statements)
  {
    (void)parse_delimiter(user_supplied_post_statements, &post_statements,
                          delimiter[0]);
  }

  if (verbose >= 2)
    printf("Parsing engines to use.\n");

  if (default_engine)
    parse_option(default_engine, &engine_options, ',');

  if (tty_password)
    opt_password= get_tty_password(NullS);
  DBUG_RETURN(0);
}