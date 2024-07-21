user_lookup (pam_handle_t *pamh, const char *database, const char *cryptmode,
	     const char *user, const char *pass, int ctrl)
{
    DBM *dbm;
    datum key, data;

    /* Open the DB file. */
    dbm = dbm_open(database, O_RDONLY, 0644);
    if (dbm == NULL) {
	pam_syslog(pamh, LOG_ERR,
		   "user_lookup: could not open database `%s': %m", database);
	return -2;
    }

    /* dump out the database contents for debugging */
    if (ctrl & PAM_DUMP_ARG) {
	pam_syslog(pamh, LOG_INFO, "Database dump:");
	for (key = dbm_firstkey(dbm);  key.dptr != NULL;
	     key = dbm_nextkey(dbm)) {
	    data = dbm_fetch(dbm, key);
	    pam_syslog(pamh, LOG_INFO,
		       "key[len=%d] = `%s', data[len=%d] = `%s'",
		       key.dsize, key.dptr, data.dsize, data.dptr);
	}
    }

    /* do some more init work */
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    if (ctrl & PAM_KEY_ONLY_ARG) {
	if (asprintf(&key.dptr, "%s-%s", user, pass) < 0)
	    key.dptr = NULL;
	else
	    key.dsize = strlen(key.dptr);
    } else {
        key.dptr = x_strdup(user);
        key.dsize = strlen(user);
    }

    if (key.dptr) {
	data = dbm_fetch(dbm, key);
	memset(key.dptr, 0, key.dsize);
	free(key.dptr);
    }

    if (ctrl & PAM_DEBUG_ARG) {
	pam_syslog(pamh, LOG_INFO,
		   "password in database is [%p]`%.*s', len is %d",
		   data.dptr, data.dsize, (char *) data.dptr, data.dsize);
    }

    if (data.dptr != NULL) {
	int compare = 0;

	if (ctrl & PAM_KEY_ONLY_ARG)
	  {
	    dbm_close (dbm);
	    return 0; /* found it, data contents don't matter */
	}

	if (cryptmode && strncasecmp(cryptmode, "crypt", 5) == 0) {

	  /* crypt(3) password storage */

	  char *cryptpw;

	  if (data.dsize < 13) {
	    compare = -2;
	  } else if (ctrl & PAM_ICASE_ARG) {
	    compare = -2;
	  } else {
	    cryptpw = crypt (pass, data.dptr);

	    if (cryptpw) {
	      compare = strncasecmp (data.dptr, cryptpw, data.dsize);
	    } else {
	      compare = -2;
	      if (ctrl & PAM_DEBUG_ARG) {
		pam_syslog(pamh, LOG_INFO, "crypt() returned NULL");
	      }
	    };

	  };

	} else {

	  /* Unknown password encryption method -
	   * default to plaintext password storage
	   */

	  if (strlen(pass) != (size_t)data.dsize) {
	    compare = 1; /* wrong password len -> wrong password */
	  } else if (ctrl & PAM_ICASE_ARG) {
	    compare = strncasecmp(data.dptr, pass, data.dsize);
	  } else {
	    compare = strncmp(data.dptr, pass, data.dsize);
	  }

	  if (cryptmode && strncasecmp(cryptmode, "none", 4)
		&& (ctrl & PAM_DEBUG_ARG)) {
	    pam_syslog(pamh, LOG_INFO, "invalid value for crypt parameter: %s",
		       cryptmode);
	    pam_syslog(pamh, LOG_INFO, "defaulting to plaintext password mode");
	  }

	}

	dbm_close(dbm);
	if (compare == 0)
	    return 0; /* match */
	else
	    return -1; /* wrong */
    } else {
        int saw_user = 0;

	if (ctrl & PAM_DEBUG_ARG) {
	    pam_syslog(pamh, LOG_INFO, "error returned by dbm_fetch: %m");
	}

	/* probably we should check dbm_error() here */

        if ((ctrl & PAM_KEY_ONLY_ARG) == 0) {
	    dbm_close(dbm);
            return 1; /* not key_only, so no entry => no entry for the user */
        }

        /* now handle the key_only case */
        for (key = dbm_firstkey(dbm);
             key.dptr != NULL;
             key = dbm_nextkey(dbm)) {
            int compare;
            /* first compare the user portion (case sensitive) */
            compare = strncmp(key.dptr, user, strlen(user));
            if (compare == 0) {
                /* assume failure */
                compare = -1;
                /* if we have the divider where we expect it to be... */
                if (key.dptr[strlen(user)] == '-') {
		    saw_user = 1;
		    if ((size_t)key.dsize == strlen(user) + 1 + strlen(pass)) {
		        if (ctrl & PAM_ICASE_ARG) {
			    /* compare the password portion (case insensitive)*/
                            compare = strncasecmp(key.dptr + strlen(user) + 1,
                                                  pass,
                                                  strlen(pass));
		        } else {
                            /* compare the password portion (case sensitive) */
                            compare = strncmp(key.dptr + strlen(user) + 1,
                                              pass,
                                              strlen(pass));
		        }
		    }
                }
                if (compare == 0) {
                    dbm_close(dbm);
                    return 0; /* match */
                }
            }
        }
        dbm_close(dbm);
	if (saw_user)
	    return -1; /* saw the user, but password mismatch */
	else
	    return 1; /* not found */
    }

    /* NOT REACHED */
    return -2;
}