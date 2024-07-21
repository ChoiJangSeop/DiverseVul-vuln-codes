int main(int argc, char **argv)
{
  int error;
  my_bool first_argument_uses_wildcards=0;
  char *wild;
  MYSQL mysql;
  MY_INIT(argv[0]);
  if (load_defaults("my",load_default_groups,&argc,&argv))
    exit(1);

  get_options(&argc,&argv);

  wild=0;
  if (argc)
  {
    char *pos= argv[argc-1], *to;
    for (to= pos ; *pos ; pos++, to++)
    {
      switch (*pos) {
      case '*':
	*pos= '%';
	first_argument_uses_wildcards= 1;
	break;
      case '?':
	*pos= '_';
	first_argument_uses_wildcards= 1;
	break;
      case '%':
      case '_':
	first_argument_uses_wildcards= 1;
	break;
      case '\\':
	pos++;
      default: break;
      }
      *to= *pos;
    }    
    *to= *pos; /* just to copy a '\0'  if '\\' was used */
  }
  if (first_argument_uses_wildcards)
    wild= argv[--argc];
  else if (argc == 3)			/* We only want one field */
    wild= argv[--argc];

  if (argc > 2)
  {
    fprintf(stderr,"%s: Too many arguments\n",my_progname);
    exit(1);
  }
  mysql_init(&mysql);
  if (opt_compress)
    mysql_options(&mysql,MYSQL_OPT_COMPRESS,NullS);
#ifdef HAVE_OPENSSL
  if (opt_use_ssl)
    mysql_ssl_set(&mysql, opt_ssl_key, opt_ssl_cert, opt_ssl_ca,
		  opt_ssl_capath, opt_ssl_cipher);
  mysql_options(&mysql,MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
                (char*)&opt_ssl_verify_server_cert);
#endif
  if (opt_protocol)
    mysql_options(&mysql,MYSQL_OPT_PROTOCOL,(char*)&opt_protocol);
#ifdef HAVE_SMEM
  if (shared_memory_base_name)
    mysql_options(&mysql,MYSQL_SHARED_MEMORY_BASE_NAME,shared_memory_base_name);
#endif
  mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, default_charset);

  if (opt_plugin_dir && *opt_plugin_dir)
    mysql_options(&mysql, MYSQL_PLUGIN_DIR, opt_plugin_dir);

  if (opt_default_auth && *opt_default_auth)
    mysql_options(&mysql, MYSQL_DEFAULT_AUTH, opt_default_auth);

  if (using_opt_enable_cleartext_plugin)
    mysql_options(&mysql, MYSQL_ENABLE_CLEARTEXT_PLUGIN,
                  (char*)&opt_enable_cleartext_plugin);

  if (!(mysql_real_connect(&mysql,host,user,opt_password,
			   (first_argument_uses_wildcards) ? "" :
                           argv[0],opt_mysql_port,opt_mysql_unix_port,
			   0)))
  {
    fprintf(stderr,"%s: %s\n",my_progname,mysql_error(&mysql));
    exit(1);
  }
  mysql.reconnect= 1;

  switch (argc) {
  case 0:  error=list_dbs(&mysql,wild); break;
  case 1:
    if (opt_status)
      error=list_table_status(&mysql,argv[0],wild);
    else
      error=list_tables(&mysql,argv[0],wild);
    break;
  default:
    if (opt_status && ! wild)
      error=list_table_status(&mysql,argv[0],argv[1]);
    else
      error=list_fields(&mysql,argv[0],argv[1],wild);
    break;
  }
  mysql_close(&mysql);			/* Close & free connection */
  my_free(opt_password);
#ifdef HAVE_SMEM
  my_free(shared_memory_base_name);
#endif
  my_end(my_end_arg);
  exit(error ? 1 : 0);
  return 0;				/* No compiler warnings */
}