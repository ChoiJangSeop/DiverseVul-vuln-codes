static int sql_auxprop_store(void *glob_context,
			     sasl_server_params_t *sparams,
			     struct propctx *ctx,
			     const char *user,
			     unsigned ulen) 
{
    char *userid = NULL;
    char *realm = NULL;
    const char *user_realm = NULL;
    int ret = SASL_FAIL;
    const struct propval *to_store, *cur;
    
    char *user_buf;
    char *statement = NULL;
    char *escap_userid = NULL;
    char *escap_realm = NULL;
    const char *cmd;
    
    sql_settings_t *settings;
    void *conn = NULL;
    
    settings = (sql_settings_t *) glob_context; 

    /* just checking if we are enabled */
    if (!ctx &&
	sql_exists(settings->sql_insert) &&
	sql_exists(settings->sql_update)) return SASL_OK;
    
    /* make sure our input is okay */
    if (!glob_context || !sparams || !user) return SASL_BADPARAM;
    
    sparams->utils->log(sparams->utils->conn, SASL_LOG_DEBUG,
			"sql plugin Parse the username %s\n", user);
    
    user_buf = sparams->utils->malloc(ulen + 1);
    if (!user_buf) {
	ret = SASL_NOMEM;
	goto done;
    }
    
    memcpy(user_buf, user, ulen);
    user_buf[ulen] = '\0';
    
    if (sparams->user_realm) {
	user_realm = sparams->user_realm;
    }
    else {
	user_realm = sparams->serverFQDN;
    }
    
    ret = _plug_parseuser(sparams->utils, &userid, &realm, user_realm,
			  sparams->serverFQDN, user_buf);
    if (ret != SASL_OK)	goto done;
    
    /* just need to escape userid and realm now */
    /* allocate some memory */
    
    escap_userid = (char *) sparams->utils->malloc(strlen(userid)*2+1);
    escap_realm = (char *) sparams->utils->malloc(strlen(realm)*2+1);
    
    if (!escap_userid || !escap_realm) {
	MEMERROR(sparams->utils);
	goto done;
    }
    
    to_store = sparams->utils->prop_get(ctx);
    
    if (!to_store) {
	ret = SASL_BADPARAM;
	goto done;
    }

    conn = sql_connect(settings, sparams->utils);
    if (!conn) {
	sparams->utils->log(sparams->utils->conn, SASL_LOG_ERR,
			    "sql plugin couldn't connect to any host\n");
	goto done;
    }
    
    settings->sql_engine->sql_escape_str(escap_userid, userid);
    settings->sql_engine->sql_escape_str(escap_realm, realm);
    
    if (settings->sql_engine->sql_begin_txn(conn, sparams->utils)) {
	sparams->utils->log(sparams->utils->conn, SASL_LOG_ERR,
			    "Unable to begin transaction\n");
    }
    for (cur = to_store; ret == SASL_OK && cur->name; cur++) {

	if (cur->name[0] == '*') {
	    continue;
	}

	/* determine which command we need */
	/* see if we already have a row for this user */
	statement = sql_create_statement(settings->sql_select,
					 SQL_WILDCARD, escap_userid,
					 escap_realm, NULL,
					 sparams->utils);
	if (!settings->sql_engine->sql_exec(conn, statement, NULL, 0, NULL,
					    sparams->utils)) {
	    /* already have a row => UPDATE */
	    cmd = settings->sql_update;
	} else {
	    /* new row => INSERT */
	    cmd = settings->sql_insert;
	}
	sparams->utils->free(statement);

	/* create a statement that we will use */
	statement = sql_create_statement(cmd, cur->name, escap_userid,
					 escap_realm,
					 cur->values && cur->values[0] ?
					 cur->values[0] : SQL_NULL_VALUE,
					 sparams->utils);
	
	{
	    char *log_statement =
		sql_create_statement(cmd, cur->name,
				     escap_userid,
				     escap_realm,
				     cur->values && cur->values[0] ?
				     "<omitted>" : SQL_NULL_VALUE,
				     sparams->utils);
	    sparams->utils->log(sparams->utils->conn, SASL_LOG_DEBUG,
				"sql plugin doing statement %s\n",
				log_statement);
	    sparams->utils->free(log_statement);
	}
	
	/* run the statement */
	if (settings->sql_engine->sql_exec(conn, statement, NULL, 0, NULL,
					   sparams->utils)) {
	    ret = SASL_FAIL;
	}
	
	sparams->utils->free(statement);
    }
    if (ret != SASL_OK) {
	sparams->utils->log(sparams->utils->conn, SASL_LOG_ERR,
			    "Failed to store auxprop; aborting transaction\n");
	if (settings->sql_engine->sql_rollback_txn(conn, sparams->utils)) {
	    sparams->utils->log(sparams->utils->conn, SASL_LOG_ERR,
				"Unable to rollback transaction\n");
	}
    }
    else if (settings->sql_engine->sql_commit_txn(conn, sparams->utils)) {
	sparams->utils->log(sparams->utils->conn, SASL_LOG_ERR,
			    "Unable to commit transaction\n");
    }
    
  done:
    if (escap_userid) sparams->utils->free(escap_userid);
    if (escap_realm) sparams->utils->free(escap_realm);
    if (conn) settings->sql_engine->sql_close(conn);
    if (userid) sparams->utils->free(userid);
    if (realm) sparams->utils->free(realm);
    if (user_buf) sparams->utils->free(user_buf);
    
    return ret;
    
    /* do a little dance */
}