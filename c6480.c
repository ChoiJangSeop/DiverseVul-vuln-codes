read_cupsd_conf(cups_file_t *fp)	/* I - File to read from */
{
  int			linenum;	/* Current line number */
  char			line[HTTP_MAX_BUFFER],
					/* Line from file */
			temp[HTTP_MAX_BUFFER],
					/* Temporary buffer for value */
			*value,		/* Pointer to value */
			*valueptr;	/* Pointer into value */
  int			valuelen;	/* Length of value */
  http_addrlist_t	*addrlist,	/* Address list */
			*addr;		/* Current address */
  cups_file_t		*incfile;	/* Include file */
  char			incname[1024];	/* Include filename */


 /*
  * Loop through each line in the file...
  */

  linenum = 0;

  while (cupsFileGetConf(fp, line, sizeof(line), &value, &linenum))
  {
   /*
    * Decode the directive...
    */

    if (!_cups_strcasecmp(line, "Include") && value)
    {
     /*
      * Include filename
      */

      if (value[0] == '/')
        strlcpy(incname, value, sizeof(incname));
      else
        snprintf(incname, sizeof(incname), "%s/%s", ServerRoot, value);

      if ((incfile = cupsFileOpen(incname, "rb")) == NULL)
        cupsdLogMessage(CUPSD_LOG_ERROR,
	                "Unable to include config file \"%s\" - %s",
	                incname, strerror(errno));
      else
      {
        read_cupsd_conf(incfile);
	cupsFileClose(incfile);
      }
    }
    else if (!_cups_strcasecmp(line, "<Location") && value)
    {
     /*
      * <Location path>
      */

      linenum = read_location(fp, value, linenum);
      if (linenum == 0)
	return (0);
    }
    else if (!_cups_strcasecmp(line, "<Policy") && value)
    {
     /*
      * <Policy name>
      */

      linenum = read_policy(fp, value, linenum);
      if (linenum == 0)
	return (0);
    }
    else if (!_cups_strcasecmp(line, "FaxRetryInterval") && value)
    {
      JobRetryInterval = atoi(value);
      cupsdLogMessage(CUPSD_LOG_WARN,
		      "FaxRetryInterval is deprecated; use "
		      "JobRetryInterval on line %d of %s.", linenum, ConfigurationFile);
    }
    else if (!_cups_strcasecmp(line, "FaxRetryLimit") && value)
    {
      JobRetryLimit = atoi(value);
      cupsdLogMessage(CUPSD_LOG_WARN,
		      "FaxRetryLimit is deprecated; use "
		      "JobRetryLimit on line %d of %s.", linenum, ConfigurationFile);
    }
#ifdef HAVE_SSL
    else if (!_cups_strcasecmp(line, "SSLOptions"))
    {
     /*
      * SSLOptions [AllowRC4] [AllowSSL3] [AllowDH] [DenyCBC] [DenyTLS1.0] [None]
      */

      int	options = _HTTP_TLS_NONE,/* SSL/TLS options */
		min_version = _HTTP_TLS_1_0,
		max_version = _HTTP_TLS_MAX;

      if (value)
      {
        char	*start,			/* Start of option */
		*end;			/* End of option */

	for (start = value; *start; start = end)
	{
	 /*
	  * Find end of keyword...
	  */

	  end = start;
	  while (*end && !_cups_isspace(*end))
	    end ++;

	  if (*end)
	    *end++ = '\0';

         /*
	  * Compare...
	  */

	  if (!_cups_strcasecmp(start, "AllowRC4"))
	    options |= _HTTP_TLS_ALLOW_RC4;
	  else if (!_cups_strcasecmp(start, "AllowSSL3"))
	    min_version = _HTTP_TLS_SSL3;
	  else if (!_cups_strcasecmp(start, "AllowDH"))
	    options |= _HTTP_TLS_ALLOW_DH;
	  else if (!_cups_strcasecmp(start, "DenyCBC"))
	    options |= _HTTP_TLS_DENY_CBC;
	  else if (!_cups_strcasecmp(start, "DenyTLS1.0"))
	    min_version = _HTTP_TLS_1_1;
	  else if (!_cups_strcasecmp(start, "MaxTLS1.0"))
	    max_version = _HTTP_TLS_1_0;
	  else if (!_cups_strcasecmp(start, "MaxTLS1.1"))
	    max_version = _HTTP_TLS_1_1;
	  else if (!_cups_strcasecmp(start, "MaxTLS1.2"))
	    max_version = _HTTP_TLS_1_2;
	  else if (!_cups_strcasecmp(start, "MaxTLS1.3"))
	    max_version = _HTTP_TLS_1_3;
	  else if (!_cups_strcasecmp(start, "MinTLS1.0"))
	    min_version = _HTTP_TLS_1_0;
	  else if (!_cups_strcasecmp(start, "MinTLS1.1"))
	    min_version = _HTTP_TLS_1_1;
	  else if (!_cups_strcasecmp(start, "MinTLS1.2"))
	    min_version = _HTTP_TLS_1_2;
	  else if (!_cups_strcasecmp(start, "MinTLS1.3"))
	    min_version = _HTTP_TLS_1_3;
	  else if (!_cups_strcasecmp(start, "None"))
	    options = _HTTP_TLS_NONE;
	  else if (_cups_strcasecmp(start, "NoEmptyFragments"))
	    cupsdLogMessage(CUPSD_LOG_WARN, "Unknown SSL option %s at line %d.", start, linenum);
        }
      }

      _httpTLSSetOptions(options, min_version, max_version);
    }
#endif /* HAVE_SSL */
    else if ((!_cups_strcasecmp(line, "Port") || !_cups_strcasecmp(line, "Listen")
#ifdef HAVE_SSL
             || !_cups_strcasecmp(line, "SSLPort") || !_cups_strcasecmp(line, "SSLListen")
#endif /* HAVE_SSL */
	     ) && value)
    {
     /*
      * Add listening address(es) to the list...
      */

      cupsd_listener_t	*lis;		/* New listeners array */


     /*
      * Get the address list...
      */

      addrlist = get_address(value, IPP_PORT);

      if (!addrlist)
      {
        cupsdLogMessage(CUPSD_LOG_ERROR, "Bad %s address %s at line %d.", line,
	                value, linenum);
        continue;
      }

     /*
      * Add each address...
      */

      for (addr = addrlist; addr; addr = addr->next)
      {
       /*
        * See if this address is already present...
	*/

        for (lis = (cupsd_listener_t *)cupsArrayFirst(Listeners);
	     lis;
	     lis = (cupsd_listener_t *)cupsArrayNext(Listeners))
          if (httpAddrEqual(&(addr->addr), &(lis->address)) &&
	      httpAddrPort(&(addr->addr)) == httpAddrPort(&(lis->address)))
	    break;

        if (lis)
	{
#ifdef HAVE_ONDEMAND
	  if (!lis->on_demand)
#endif /* HAVE_ONDEMAND */
	  {
	    httpAddrString(&lis->address, temp, sizeof(temp));
	    cupsdLogMessage(CUPSD_LOG_WARN,
			    "Duplicate listen address \"%s\" ignored.", temp);
	  }

          continue;
	}

       /*
        * Allocate another listener...
	*/

        if (!Listeners)
	  Listeners = cupsArrayNew(NULL, NULL);

	if (!Listeners)
	{
          cupsdLogMessage(CUPSD_LOG_ERROR,
	                  "Unable to allocate %s at line %d - %s.",
	                  line, linenum, strerror(errno));
          break;
	}

        if ((lis = calloc(1, sizeof(cupsd_listener_t))) == NULL)
	{
          cupsdLogMessage(CUPSD_LOG_ERROR,
	                  "Unable to allocate %s at line %d - %s.",
	                  line, linenum, strerror(errno));
          break;
	}

        cupsArrayAdd(Listeners, lis);

       /*
        * Copy the current address and log it...
	*/

	memcpy(&(lis->address), &(addr->addr), sizeof(lis->address));
	lis->fd = -1;

#ifdef HAVE_SSL
        if (!_cups_strcasecmp(line, "SSLPort") || !_cups_strcasecmp(line, "SSLListen"))
          lis->encryption = HTTP_ENCRYPT_ALWAYS;
#endif /* HAVE_SSL */

	httpAddrString(&lis->address, temp, sizeof(temp));

#ifdef AF_LOCAL
        if (lis->address.addr.sa_family == AF_LOCAL)
          cupsdLogMessage(CUPSD_LOG_INFO, "Listening to %s (Domain)", temp);
	else
#endif /* AF_LOCAL */
	cupsdLogMessage(CUPSD_LOG_INFO, "Listening to %s:%d (IPv%d)", temp,
                        httpAddrPort(&(lis->address)),
			httpAddrFamily(&(lis->address)) == AF_INET ? 4 : 6);

        if (!httpAddrLocalhost(&(lis->address)))
	  RemotePort = httpAddrPort(&(lis->address));
      }

     /*
      * Free the list...
      */

      httpAddrFreeList(addrlist);
    }
    else if (!_cups_strcasecmp(line, "BrowseProtocols") ||
             !_cups_strcasecmp(line, "BrowseLocalProtocols"))
    {
     /*
      * "BrowseProtocols name [... name]"
      * "BrowseLocalProtocols name [... name]"
      */

      int protocols = parse_protocols(value);

      if (protocols < 0)
      {
	cupsdLogMessage(CUPSD_LOG_ERROR,
	                "Unknown browse protocol \"%s\" on line %d of %s.",
	                value, linenum, ConfigurationFile);
        break;
      }

      BrowseLocalProtocols = protocols;
    }
    else if (!_cups_strcasecmp(line, "DefaultAuthType") && value)
    {
     /*
      * DefaultAuthType {basic,digest,basicdigest,negotiate}
      */

      if (!_cups_strcasecmp(value, "none"))
	default_auth_type = CUPSD_AUTH_NONE;
      else if (!_cups_strcasecmp(value, "basic"))
	default_auth_type = CUPSD_AUTH_BASIC;
      else if (!_cups_strcasecmp(value, "negotiate"))
        default_auth_type = CUPSD_AUTH_NEGOTIATE;
      else if (!_cups_strcasecmp(value, "auto"))
        default_auth_type = CUPSD_AUTH_AUTO;
      else
      {
	cupsdLogMessage(CUPSD_LOG_WARN,
	                "Unknown default authorization type %s on line %d of %s.",
	                value, linenum, ConfigurationFile);
	if (FatalErrors & CUPSD_FATAL_CONFIG)
	  return (0);
      }
    }
#ifdef HAVE_SSL
    else if (!_cups_strcasecmp(line, "DefaultEncryption"))
    {
     /*
      * DefaultEncryption {Never,IfRequested,Required}
      */

      if (!value || !_cups_strcasecmp(value, "never"))
	DefaultEncryption = HTTP_ENCRYPT_NEVER;
      else if (!_cups_strcasecmp(value, "required"))
	DefaultEncryption = HTTP_ENCRYPT_REQUIRED;
      else if (!_cups_strcasecmp(value, "ifrequested"))
	DefaultEncryption = HTTP_ENCRYPT_IF_REQUESTED;
      else
      {
	cupsdLogMessage(CUPSD_LOG_WARN,
	                "Unknown default encryption %s on line %d of %s.",
	                value, linenum, ConfigurationFile);
	if (FatalErrors & CUPSD_FATAL_CONFIG)
	  return (0);
      }
    }
#endif /* HAVE_SSL */
    else if (!_cups_strcasecmp(line, "HostNameLookups") && value)
    {
     /*
      * Do hostname lookups?
      */

      if (!_cups_strcasecmp(value, "off") || !_cups_strcasecmp(value, "no") ||
          !_cups_strcasecmp(value, "false"))
        HostNameLookups = 0;
      else if (!_cups_strcasecmp(value, "on") || !_cups_strcasecmp(value, "yes") ||
          !_cups_strcasecmp(value, "true"))
        HostNameLookups = 1;
      else if (!_cups_strcasecmp(value, "double"))
        HostNameLookups = 2;
      else
	cupsdLogMessage(CUPSD_LOG_WARN, "Unknown HostNameLookups %s on line %d of %s.",
	                value, linenum, ConfigurationFile);
    }
    else if (!_cups_strcasecmp(line, "AccessLogLevel") && value)
    {
     /*
      * Amount of logging to do to access log...
      */

      if (!_cups_strcasecmp(value, "all"))
        AccessLogLevel = CUPSD_ACCESSLOG_ALL;
      else if (!_cups_strcasecmp(value, "actions"))
        AccessLogLevel = CUPSD_ACCESSLOG_ACTIONS;
      else if (!_cups_strcasecmp(value, "config"))
        AccessLogLevel = CUPSD_ACCESSLOG_CONFIG;
      else if (!_cups_strcasecmp(value, "none"))
        AccessLogLevel = CUPSD_ACCESSLOG_NONE;
      else
        cupsdLogMessage(CUPSD_LOG_WARN, "Unknown AccessLogLevel %s on line %d of %s.",
	                value, linenum, ConfigurationFile);
    }
    else if (!_cups_strcasecmp(line, "LogLevel") && value)
    {
     /*
      * Amount of logging to do to error log...
      */

      if (!_cups_strcasecmp(value, "debug2"))
        LogLevel = CUPSD_LOG_DEBUG2;
      else if (!_cups_strcasecmp(value, "debug"))
        LogLevel = CUPSD_LOG_DEBUG;
      else if (!_cups_strcasecmp(value, "info"))
        LogLevel = CUPSD_LOG_INFO;
      else if (!_cups_strcasecmp(value, "notice"))
        LogLevel = CUPSD_LOG_NOTICE;
      else if (!_cups_strcasecmp(value, "warn"))
        LogLevel = CUPSD_LOG_WARN;
      else if (!_cups_strcasecmp(value, "error"))
        LogLevel = CUPSD_LOG_ERROR;
      else if (!_cups_strcasecmp(value, "crit"))
        LogLevel = CUPSD_LOG_CRIT;
      else if (!_cups_strcasecmp(value, "alert"))
        LogLevel = CUPSD_LOG_ALERT;
      else if (!_cups_strcasecmp(value, "emerg"))
        LogLevel = CUPSD_LOG_EMERG;
      else if (!_cups_strcasecmp(value, "none"))
        LogLevel = CUPSD_LOG_NONE;
      else
        cupsdLogMessage(CUPSD_LOG_WARN, "Unknown LogLevel %s on line %d of %s.",
	                value, linenum, ConfigurationFile);
    }
    else if (!_cups_strcasecmp(line, "LogTimeFormat") && value)
    {
     /*
      * Amount of logging to do to error log...
      */

      if (!_cups_strcasecmp(value, "standard"))
        LogTimeFormat = CUPSD_TIME_STANDARD;
      else if (!_cups_strcasecmp(value, "usecs"))
        LogTimeFormat = CUPSD_TIME_USECS;
      else
        cupsdLogMessage(CUPSD_LOG_WARN, "Unknown LogTimeFormat %s on line %d of %s.",
	                value, linenum, ConfigurationFile);
    }
    else if (!_cups_strcasecmp(line, "ServerTokens") && value)
    {
     /*
      * Set the string used for the Server header...
      */

      struct utsname plat;		/* Platform info */


      uname(&plat);

      if (!_cups_strcasecmp(value, "ProductOnly"))
	cupsdSetString(&ServerHeader, "CUPS IPP");
      else if (!_cups_strcasecmp(value, "Major"))
	cupsdSetStringf(&ServerHeader, "CUPS/%d IPP/2", CUPS_VERSION_MAJOR);
      else if (!_cups_strcasecmp(value, "Minor"))
	cupsdSetStringf(&ServerHeader, "CUPS/%d.%d IPP/2.1", CUPS_VERSION_MAJOR,
	                CUPS_VERSION_MINOR);
      else if (!_cups_strcasecmp(value, "Minimal"))
	cupsdSetString(&ServerHeader, CUPS_MINIMAL " IPP/2.1");
      else if (!_cups_strcasecmp(value, "OS"))
	cupsdSetStringf(&ServerHeader, CUPS_MINIMAL " (%s %s) IPP/2.1",
	                plat.sysname, plat.release);
      else if (!_cups_strcasecmp(value, "Full"))
	cupsdSetStringf(&ServerHeader, CUPS_MINIMAL " (%s %s; %s) IPP/2.1",
	                plat.sysname, plat.release, plat.machine);
      else if (!_cups_strcasecmp(value, "None"))
	cupsdSetString(&ServerHeader, "");
      else
	cupsdLogMessage(CUPSD_LOG_WARN, "Unknown ServerTokens %s on line %d of %s.",
                        value, linenum, ConfigurationFile);
    }
    else if (!_cups_strcasecmp(line, "PassEnv") && value)
    {
     /*
      * PassEnv variable [... variable]
      */

      for (; *value;)
      {
        for (valuelen = 0; value[valuelen]; valuelen ++)
	  if (_cups_isspace(value[valuelen]) || value[valuelen] == ',')
	    break;

        if (value[valuelen])
        {
	  value[valuelen] = '\0';
	  valuelen ++;
	}

        cupsdSetEnv(value, NULL);

        for (value += valuelen; *value; value ++)
	  if (!_cups_isspace(*value) || *value != ',')
	    break;
      }
    }
    else if (!_cups_strcasecmp(line, "ServerAlias") && value)
    {
     /*
      * ServerAlias name [... name]
      */

      if (!ServerAlias)
        ServerAlias = cupsArrayNew(NULL, NULL);

      for (; *value;)
      {
        for (valuelen = 0; value[valuelen]; valuelen ++)
	  if (_cups_isspace(value[valuelen]) || value[valuelen] == ',')
	    break;

        if (value[valuelen])
        {
	  value[valuelen] = '\0';
	  valuelen ++;
	}

	cupsdAddAlias(ServerAlias, value);

        for (value += valuelen; *value; value ++)
	  if (!_cups_isspace(*value) || *value != ',')
	    break;
      }
    }
    else if (!_cups_strcasecmp(line, "SetEnv") && value)
    {
     /*
      * SetEnv variable value
      */

      for (valueptr = value; *valueptr && !isspace(*valueptr & 255); valueptr ++);

      if (*valueptr)
      {
       /*
        * Found a value...
	*/

        while (isspace(*valueptr & 255))
	  *valueptr++ = '\0';

        cupsdSetEnv(value, valueptr);
      }
      else
        cupsdLogMessage(CUPSD_LOG_ERROR,
	                "Missing value for SetEnv directive on line %d of %s.",
	                linenum, ConfigurationFile);
    }
    else if (!_cups_strcasecmp(line, "AccessLog") ||
             !_cups_strcasecmp(line, "CacheDir") ||
             !_cups_strcasecmp(line, "ConfigFilePerm") ||
             !_cups_strcasecmp(line, "DataDir") ||
             !_cups_strcasecmp(line, "DocumentRoot") ||
             !_cups_strcasecmp(line, "ErrorLog") ||
             !_cups_strcasecmp(line, "FatalErrors") ||
             !_cups_strcasecmp(line, "FileDevice") ||
             !_cups_strcasecmp(line, "FontPath") ||
             !_cups_strcasecmp(line, "Group") ||
             !_cups_strcasecmp(line, "LogFilePerm") ||
             !_cups_strcasecmp(line, "LPDConfigFile") ||
             !_cups_strcasecmp(line, "PageLog") ||
             !_cups_strcasecmp(line, "Printcap") ||
             !_cups_strcasecmp(line, "PrintcapFormat") ||
             !_cups_strcasecmp(line, "RemoteRoot") ||
             !_cups_strcasecmp(line, "RequestRoot") ||
             !_cups_strcasecmp(line, "ServerBin") ||
             !_cups_strcasecmp(line, "ServerCertificate") ||
             !_cups_strcasecmp(line, "ServerKey") ||
             !_cups_strcasecmp(line, "ServerKeychain") ||
             !_cups_strcasecmp(line, "ServerRoot") ||
             !_cups_strcasecmp(line, "SMBConfigFile") ||
             !_cups_strcasecmp(line, "StateDir") ||
             !_cups_strcasecmp(line, "SystemGroup") ||
             !_cups_strcasecmp(line, "SystemGroupAuthKey") ||
             !_cups_strcasecmp(line, "TempDir") ||
	     !_cups_strcasecmp(line, "User"))
    {
      cupsdLogMessage(CUPSD_LOG_INFO,
		      "Please move \"%s%s%s\" on line %d of %s to the %s file; "
		      "this will become an error in a future release.",
		      line, value ? " " : "", value ? value : "", linenum,
		      ConfigurationFile, CupsFilesFile);
    }
    else
      parse_variable(ConfigurationFile, linenum, line, value,
                     sizeof(cupsd_vars) / sizeof(cupsd_vars[0]), cupsd_vars);
  }

  return (1);
}