int Lua::handle_script_request(struct mg_connection *conn,
			       const struct mg_request_info *request_info,
			       char *script_path) {
  char buf[64], key[64], ifname[MAX_INTERFACE_NAME_LEN];
  char *_cookies, user[64] = { '\0' }, outbuf[FILENAME_MAX];
  patricia_tree_t *ptree = NULL;
  int rc;

  if(!L) return(-1);

  luaL_openlibs(L); /* Load base libraries */
  lua_register_classes(L, true); /* Load custom classes */

  lua_pushlightuserdata(L, (char*)conn);
  lua_setglobal(L, CONST_HTTP_CONN);

  /* Put the GET params into the environment */
  lua_newtable(L);
  if(request_info->query_string != NULL) {
    char *query_string = strdup(request_info->query_string);

    if(query_string) {
      char *where;
      char *tok;

      // ntop->getTrace()->traceEvent(TRACE_WARNING, "[HTTP] %s", query_string);

      tok = strtok_r(query_string, "&", &where);

      while(tok != NULL) {
	/* key=val */
	char *_equal = strchr(tok, '=');

	if(_equal) {
	  char *equal;
	  int len;
	  
	  _equal[0] = '\0';
	  _equal = &_equal[1];
	  len = strlen(_equal);

	  purifyHTTPParameter(tok), purifyHTTPParameter(_equal);

	  if((equal = (char*)malloc(len+1)) != NULL) {
	    char *decoded_buf;

	    Utils::urlDecode(_equal, equal, len+1);

	    if((decoded_buf = http_decode(equal)) != NULL) {
	      FILE *fd;

	      Utils::purifyHTTPparam(tok, true);
	      Utils::purifyHTTPparam(decoded_buf, false);

	      /* Now make sure that decoded_buf is not a file path */
	      if((decoded_buf[0] == '.')
		 && ((fd = fopen(decoded_buf, "r"))
		     || (fd = fopen(realpath(decoded_buf, outbuf), "r")))) {
		ntop->getTrace()->traceEvent(TRACE_WARNING, "Discarded '%s'='%s' as argument is a valid file path",
					     tok, decoded_buf);

		decoded_buf[0] = '\0';
		fclose(fd);
	      }

	      /* ntop->getTrace()->traceEvent(TRACE_WARNING, "'%s'='%s'", tok, decoded_buf); */

	      if(strcmp(tok, "csrf") == 0) {
		char rsp[32];
		if(ntop->getRedis()->get(decoded_buf, rsp, sizeof(rsp)) == -1) {
		  ntop->getTrace()->traceEvent(TRACE_WARNING, "Invalid CSRF parameter specified [%s]", decoded_buf);
		  free(equal);
		  return(send_error(conn, 500 /* Internal server error */, "Internal server error: CSRF attack?", PAGE_ERROR, tok, rsp));
		}
	      }

	      lua_push_str_table_entry(L, tok, decoded_buf);
	      free(decoded_buf);
	    }

	    free(equal);
	  } else
	    ntop->getTrace()->traceEvent(TRACE_WARNING, "Not enough memory");
	}

	tok = strtok_r(NULL, "&", &where);
      } /* while */

      free(query_string);
    } else
      ntop->getTrace()->traceEvent(TRACE_WARNING, "Not enough memory");
  }
  lua_setglobal(L, "_GET"); /* Like in php */

  /* _SERVER */
  lua_newtable(L);
  lua_push_str_table_entry(L, "HTTP_REFERER", (char*)mg_get_header(conn, "Referer"));
  lua_push_str_table_entry(L, "HTTP_USER_AGENT", (char*)mg_get_header(conn, "User-Agent"));
  lua_push_str_table_entry(L, "SERVER_NAME", (char*)mg_get_header(conn, "Host"));
  lua_setglobal(L, "_SERVER"); /* Like in php */

  /* Cookies */
  lua_newtable(L);
  if((_cookies = (char*)mg_get_header(conn, "Cookie")) != NULL) {
    char *cookies = strdup(_cookies);
    char *tok, *where;

    // ntop->getTrace()->traceEvent(TRACE_WARNING, "=> '%s'", cookies);
    tok = strtok_r(cookies, "=", &where);
    while(tok != NULL) {
      char *val;

      while(tok[0] == ' ') tok++;
      
      if((val = strtok_r(NULL, ";", &where)) != NULL) {
	lua_push_str_table_entry(L, tok, val);
	// ntop->getTrace()->traceEvent(TRACE_WARNING, "'%s'='%s'", tok, val);
      } else
	break;

      tok = strtok_r(NULL, "=", &where);
    }

    free(cookies);
  }
  lua_setglobal(L, "_COOKIE"); /* Like in php */

  /* Put the _SESSION params into the environment */
  lua_newtable(L);

  mg_get_cookie(conn, "user", user, sizeof(user));
  lua_push_str_table_entry(L, "user", user);
  mg_get_cookie(conn, "session", buf, sizeof(buf));
  lua_push_str_table_entry(L, "session", buf);

  if(user[0] != '\0') {
    snprintf(key, sizeof(key), "ntopng.prefs.%s.ifname", user);
    if(ntop->getRedis()->get(key, ifname, sizeof(ifname)) < 0) {
    set_default_if_name_in_session:
      snprintf(ifname, sizeof(ifname), "%s", ntop->getInterfaceAtId(0)->get_name());
      lua_push_str_table_entry(L, "ifname", ifname);
      ntop->getRedis()->set(key, ifname, 3600 /* 1h */);
    } else {
      goto set_preferred_interface;
    }
  } else {
    // We need to check if ntopng is running with the option --disable-login
    snprintf(key, sizeof(key), "ntopng.prefs.ifname");
    if(ntop->getRedis()->get(key, ifname, sizeof(ifname)) < 0) {
      goto set_preferred_interface;
    }

  set_preferred_interface:
    NetworkInterfaceView *iface;

    if((iface = ntop->getInterfaceView(ifname)) != NULL) {
      /* The specified interface still exists */
      lua_push_str_table_entry(L, "ifname", iface->get_name());
    } else {
      goto set_default_if_name_in_session;
    }
  }

  lua_setglobal(L, "_SESSION"); /* Like in php */

  if(user[0] != '\0') {
    char val[255];

    lua_pushlightuserdata(L, user);
    lua_setglobal(L, "user");

    snprintf(key, sizeof(key), "ntopng.user.%s.allowed_nets", user);

    if((ntop->getRedis()->get(key, val, sizeof(val)) != -1)
       && (val[0] != '\0')) {
      char *what, *net;

      // ntop->getTrace()->traceEvent(TRACE_WARNING, "%s", val);

      ptree = New_Patricia(128);
      net = strtok_r(val, ",", &what);

      while(net != NULL) {
	// ntop->getTrace()->traceEvent(TRACE_ERROR, "%s", net);
	ptree_add_rule(ptree, net);
	net = strtok_r(NULL, ",", &what);
      }

      lua_pushlightuserdata(L, ptree);
      lua_setglobal(L, CONST_ALLOWED_NETS);
    }
  }

#ifndef NTOPNG_PRO
  rc = luaL_dofile(L, script_path);
#else
  if(ntop->getPro()->has_valid_license())
    rc = __ntop_lua_handlefile(L, script_path, true);
  else
    rc = luaL_dofile(L, script_path);
#endif

  if(rc != 0) {
    const char *err = lua_tostring(L, -1);

    if(ptree) Destroy_Patricia(ptree, free_ptree_data);

    ntop->getTrace()->traceEvent(TRACE_WARNING, "Script failure [%s][%s]", script_path, err);
    return(send_error(conn, 500 /* Internal server error */,
		      "Internal server error", PAGE_ERROR, script_path, err));
  }

  if(ptree) Destroy_Patricia(ptree, free_ptree_data);
  return(CONST_LUA_OK);
}