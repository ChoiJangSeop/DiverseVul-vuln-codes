handle_nsExtendConfigTable(netsnmp_mib_handler          *handler,
                     netsnmp_handler_registration *reginfo,
                     netsnmp_agent_request_info   *reqinfo,
                     netsnmp_request_info         *requests)
{
    netsnmp_request_info       *request;
    netsnmp_table_request_info *table_info;
    netsnmp_extend             *extension;
    extend_registration_block  *eptr;
    int  i;
    int  need_to_validate = 0;

    for ( request=requests; request; request=request->next ) {
        if (request->processed)
            continue;
        table_info = netsnmp_extract_table_info( request );
        extension  = (netsnmp_extend*)netsnmp_extract_table_row_data( request );

        DEBUGMSGTL(( "nsExtendTable:config", "varbind: "));
        DEBUGMSGOID(("nsExtendTable:config", request->requestvb->name,
                                             request->requestvb->name_length));
        DEBUGMSG((   "nsExtendTable:config", " (%s)\n",
                      se_find_label_in_slist("agent_mode", reqinfo->mode)));

        switch (reqinfo->mode) {
        case MODE_GET:
            switch (table_info->colnum) {
            case COLUMN_EXTCFG_COMMAND:
                snmp_set_var_typed_value(
                     request->requestvb, ASN_OCTET_STR,
                     extension->command,
                    (extension->command)?strlen(extension->command):0);
                break;
            case COLUMN_EXTCFG_ARGS:
                snmp_set_var_typed_value(
                     request->requestvb, ASN_OCTET_STR,
                     extension->args,
                    (extension->args)?strlen(extension->args):0);
                break;
            case COLUMN_EXTCFG_INPUT:
                snmp_set_var_typed_value(
                     request->requestvb, ASN_OCTET_STR,
                     extension->input,
                    (extension->input)?strlen(extension->input):0);
                break;
            case COLUMN_EXTCFG_CACHETIME:
                snmp_set_var_typed_value(
                     request->requestvb, ASN_INTEGER,
                    (u_char*)&extension->cache->timeout, sizeof(int));
                break;
            case COLUMN_EXTCFG_EXECTYPE:
                i = ((extension->flags & NS_EXTEND_FLAGS_SHELL) ?
                                         NS_EXTEND_ETYPE_SHELL :
                                         NS_EXTEND_ETYPE_EXEC);
                snmp_set_var_typed_value(
                     request->requestvb, ASN_INTEGER,
                    (u_char*)&i, sizeof(i));
                break;
            case COLUMN_EXTCFG_RUNTYPE:
                i = ((extension->flags & NS_EXTEND_FLAGS_WRITEABLE) ?
                                         NS_EXTEND_RTYPE_RWRITE :
                                         NS_EXTEND_RTYPE_RONLY);
                snmp_set_var_typed_value(
                     request->requestvb, ASN_INTEGER,
                    (u_char*)&i, sizeof(i));
                break;

            case COLUMN_EXTCFG_STORAGE:
                i = ((extension->flags & NS_EXTEND_FLAGS_CONFIG) ?
                                         ST_PERMANENT : ST_VOLATILE);
                snmp_set_var_typed_value(
                     request->requestvb, ASN_INTEGER,
                    (u_char*)&i, sizeof(i));
                break;
            case COLUMN_EXTCFG_STATUS:
                i = ((extension->flags & NS_EXTEND_FLAGS_ACTIVE) ?
                                         RS_ACTIVE :
                                         RS_NOTINSERVICE);
                snmp_set_var_typed_value(
                     request->requestvb, ASN_INTEGER,
                    (u_char*)&i, sizeof(i));
                break;

            default:
                netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHOBJECT);
                continue;
            }
            break;

        /**********
         *
         * Start of SET handling
         *
         *   All config objects are potentially writable except
         *     nsExtendStorage which is fixed as either 'permanent'
         *     (if read from a config file) or 'volatile' (if set via SNMP)
         *   The string-based settings of a 'permanent' entry cannot 
         *     be changed - neither can the execution or run type.
         *   Such entries can be (temporarily) marked as inactive,
         *     and the cache timeout adjusted, but these changes are
         *     not persistent.
         *
         **********/

#ifndef NETSNMP_NO_WRITE_SUPPORT
        case MODE_SET_RESERVE1:
            /*
             * Validate the new assignments
             */
            switch (table_info->colnum) {
            case COLUMN_EXTCFG_COMMAND:
                if (request->requestvb->type != ASN_OCTET_STR) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_WRONGTYPE);
                    return SNMP_ERR_WRONGTYPE;
                }
                /*
                 * Must have a full path to the command
                 * XXX - Assumes Unix-style paths
                 */
                if (request->requestvb->val_len == 0 ||
                    request->requestvb->val.string[0] != '/') {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_WRONGVALUE);
                    return SNMP_ERR_WRONGVALUE;
                }
                /*
                 * XXX - need to check this file exists
                 *       (and is executable)
                 */

                if (extension && extension->flags & NS_EXTEND_FLAGS_CONFIG) {
                    /*
                     * config entries are "permanent" so can't be changed
                     */
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_NOTWRITABLE);
                    return SNMP_ERR_NOTWRITABLE;
                }
                break;

            case COLUMN_EXTCFG_ARGS:
            case COLUMN_EXTCFG_INPUT:
                if (request->requestvb->type != ASN_OCTET_STR) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_WRONGTYPE);
                    return SNMP_ERR_WRONGTYPE;
                }

                if (extension && extension->flags & NS_EXTEND_FLAGS_CONFIG) {
                    /*
                     * config entries are "permanent" so can't be changed
                     */
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_NOTWRITABLE);
                    return SNMP_ERR_NOTWRITABLE;
                }
                break;

            case COLUMN_EXTCFG_CACHETIME:
                if (request->requestvb->type != ASN_INTEGER) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_WRONGTYPE);
                    return SNMP_ERR_WRONGTYPE;
                }
                i = *request->requestvb->val.integer;
                /*
                 * -1 is a special value indicating "don't cache"
                 *    [[ XXX - should this be 0 ?? ]]
                 * Otherwise, cache times must be non-negative
                 */
                if (i < -1 ) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_WRONGVALUE);
                    return SNMP_ERR_WRONGVALUE;
                }
                break;

            case COLUMN_EXTCFG_EXECTYPE:
                if (request->requestvb->type != ASN_INTEGER) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_WRONGTYPE);
                    return SNMP_ERR_WRONGTYPE;
                }
                i = *request->requestvb->val.integer;
                if (i<1 || i>2) {  /* 'exec(1)' or 'shell(2)' only */
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_WRONGVALUE);
                    return SNMP_ERR_WRONGVALUE;
                }
                if (extension && extension->flags & NS_EXTEND_FLAGS_CONFIG) {
                    /*
                     * config entries are "permanent" so can't be changed
                     */
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_NOTWRITABLE);
                    return SNMP_ERR_NOTWRITABLE;
                }
                break;

            case COLUMN_EXTCFG_RUNTYPE:
                if (request->requestvb->type != ASN_INTEGER) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_WRONGTYPE);
                    return SNMP_ERR_WRONGTYPE;
                }
                /*
                 * 'run-on-read(1)', 'run-on-set(2)'
                 *  or 'run-command(3)' only
                 */
                i = *request->requestvb->val.integer;
                if (i<1 || i>3) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_WRONGVALUE);
                    return SNMP_ERR_WRONGVALUE;
                }
                /*
                 * 'run-command(3)' can only be used with
                 *  a pre-existing 'run-on-set(2)' entry.
                 */
                if (i==3 && !(extension && (extension->flags & NS_EXTEND_FLAGS_WRITEABLE))) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_INCONSISTENTVALUE);
                    return SNMP_ERR_INCONSISTENTVALUE;
                }
                /*
                 * 'run-command(3)' is the only valid assignment
                 *  for permanent (i.e. config) entries
                 */
                if ((extension && extension->flags & NS_EXTEND_FLAGS_CONFIG)
                    && i!=3 ) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_INCONSISTENTVALUE);
                    return SNMP_ERR_INCONSISTENTVALUE;
                }
                break;

            case COLUMN_EXTCFG_STATUS:
                if (request->requestvb->type != ASN_INTEGER) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_WRONGTYPE);
                    return SNMP_ERR_WRONGTYPE;
                }
                i = *request->requestvb->val.integer;
                switch (i) {
                case RS_ACTIVE:
                case RS_NOTINSERVICE:
                    if (!extension) {
                        /* Must be used with existing rows */
                        netsnmp_set_request_error(reqinfo, request,
                                                  SNMP_ERR_INCONSISTENTVALUE);
                        return SNMP_ERR_INCONSISTENTVALUE;
                    }
                    break;    /* OK */
                case RS_CREATEANDGO:
                case RS_CREATEANDWAIT:
                    if (extension) {
                        /* Can only be used to create new rows */
                        netsnmp_set_request_error(reqinfo, request,
                                                  SNMP_ERR_INCONSISTENTVALUE);
                        return SNMP_ERR_INCONSISTENTVALUE;
                    }
                    break;
                case RS_DESTROY:
                    break;
                default:
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_WRONGVALUE);
                    return SNMP_ERR_WRONGVALUE;
                }
                break;

            default:
                netsnmp_set_request_error(reqinfo, request,
                                          SNMP_ERR_NOTWRITABLE);
                return SNMP_ERR_NOTWRITABLE;
            }
            break;

        case MODE_SET_RESERVE2:
            switch (table_info->colnum) {
            case COLUMN_EXTCFG_STATUS:
                i = *request->requestvb->val.integer;
                switch (i) {
                case RS_CREATEANDGO:
                case RS_CREATEANDWAIT:
                    eptr = _find_extension_block( request->requestvb->name,
                                                  request->requestvb->name_length );
                    extension = _new_extension( (char *) table_info->indexes->val.string,
                                                0, eptr );
                    if (!extension) {  /* failed */
                        netsnmp_set_request_error(reqinfo, request,
                                                  SNMP_ERR_RESOURCEUNAVAILABLE);
                        return SNMP_ERR_RESOURCEUNAVAILABLE;
                    }
                    netsnmp_insert_table_row( request, extension->row );
                }
            }
            break;

        case MODE_SET_FREE:
            switch (table_info->colnum) {
            case COLUMN_EXTCFG_STATUS:
                i = *request->requestvb->val.integer;
                switch (i) {
                case RS_CREATEANDGO:
                case RS_CREATEANDWAIT:
                    eptr = _find_extension_block( request->requestvb->name,
                                                  request->requestvb->name_length );
                    _free_extension( extension, eptr );
                }
            }
            break;

        case MODE_SET_ACTION:
            switch (table_info->colnum) {
            case COLUMN_EXTCFG_COMMAND:
                extension->old_command = extension->command;
                extension->command = netsnmp_strdup_and_null(
                    request->requestvb->val.string,
                    request->requestvb->val_len);
                break;
            case COLUMN_EXTCFG_ARGS:
                extension->old_args = extension->args;
                extension->args = netsnmp_strdup_and_null(
                    request->requestvb->val.string,
                    request->requestvb->val_len);
                break;
            case COLUMN_EXTCFG_INPUT:
                extension->old_input = extension->input;
                extension->input = netsnmp_strdup_and_null(
                    request->requestvb->val.string,
                    request->requestvb->val_len);
                break;
            case COLUMN_EXTCFG_STATUS:
                i = *request->requestvb->val.integer;
                switch (i) {
                case RS_ACTIVE:
                case RS_CREATEANDGO:
                    need_to_validate = 1;
                }
                break;
            }
            break;

        case MODE_SET_UNDO:
            switch (table_info->colnum) {
            case COLUMN_EXTCFG_COMMAND:
                if ( extension && extension->old_command ) {
                    SNMP_FREE(extension->command);
                    extension->command     = extension->old_command;
                    extension->old_command = NULL;
                }
                break;
            case COLUMN_EXTCFG_ARGS:
                if ( extension && extension->old_args ) {
                    SNMP_FREE(extension->args);
                    extension->args     = extension->old_args;
                    extension->old_args = NULL;
                }
                break;
            case COLUMN_EXTCFG_INPUT:
                if ( extension && extension->old_input ) {
                    SNMP_FREE(extension->input);
                    extension->input     = extension->old_input;
                    extension->old_input = NULL;
                }
                break;
            case COLUMN_EXTCFG_STATUS:
                i = *request->requestvb->val.integer;
                switch (i) {
                case RS_CREATEANDGO:
                case RS_CREATEANDWAIT:
                    eptr = _find_extension_block( request->requestvb->name,
                                                  request->requestvb->name_length );
                    _free_extension( extension, eptr );
                }
                break;
            }
            break;

        case MODE_SET_COMMIT:
            switch (table_info->colnum) {
            case COLUMN_EXTCFG_CACHETIME:
                i = *request->requestvb->val.integer;
                extension->cache->timeout = i;
                break;

            case COLUMN_EXTCFG_RUNTYPE:
                i = *request->requestvb->val.integer;
                switch (i) {
                case 1:
                    extension->flags &= ~NS_EXTEND_FLAGS_WRITEABLE;
                    break;
                case 2:
                    extension->flags |=  NS_EXTEND_FLAGS_WRITEABLE;
                    break;
                case 3:
                    (void)netsnmp_cache_check_and_reload( extension->cache );
                    break;
                }
                break;

            case COLUMN_EXTCFG_EXECTYPE:
                i = *request->requestvb->val.integer;
                if ( i == NS_EXTEND_ETYPE_SHELL )
                    extension->flags |=  NS_EXTEND_FLAGS_SHELL;
                else
                    extension->flags &= ~NS_EXTEND_FLAGS_SHELL;
                break;

            case COLUMN_EXTCFG_STATUS:
                i = *request->requestvb->val.integer;
                switch (i) {
                case RS_ACTIVE:
                case RS_CREATEANDGO:
                    extension->flags |= NS_EXTEND_FLAGS_ACTIVE;
                    break;
                case RS_NOTINSERVICE:
                case RS_CREATEANDWAIT:
                    extension->flags &= ~NS_EXTEND_FLAGS_ACTIVE;
                    break;
                case RS_DESTROY:
                    eptr = _find_extension_block( request->requestvb->name,
                                                  request->requestvb->name_length );
                    _free_extension( extension, eptr );
                    break;
                }
            }
            break;
#endif /* !NETSNMP_NO_WRITE_SUPPORT */ 

        default:
            netsnmp_set_request_error(reqinfo, request, SNMP_ERR_GENERR);
            return SNMP_ERR_GENERR;
        }
    }

#ifndef NETSNMP_NO_WRITE_SUPPORT
    /*
     * If we're marking a given row as active,
     *  then we need to check that it's ready.
     */
    if (need_to_validate) {
        for ( request=requests; request; request=request->next ) {
            if (request->processed)
                continue;
            table_info = netsnmp_extract_table_info( request );
            extension  = (netsnmp_extend*)netsnmp_extract_table_row_data( request );
            switch (table_info->colnum) {
            case COLUMN_EXTCFG_STATUS:
                i = *request->requestvb->val.integer;
                if (( i == RS_ACTIVE || i == RS_CREATEANDGO ) &&
                    !(extension && extension->command &&
                      extension->command[0] == '/' /* &&
                      is_executable(extension->command) */)) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_ERR_INCONSISTENTVALUE);
                    return SNMP_ERR_INCONSISTENTVALUE;
                }
            }
        }
    }
#endif /* !NETSNMP_NO_WRITE_SUPPORT */
    
    return SNMP_ERR_NOERROR;
}