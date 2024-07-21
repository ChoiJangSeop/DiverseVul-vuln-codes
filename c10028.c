NOEXPORT int init_section(int eof, SERVICE_OPTIONS **section_ptr) {
    char *errstr;

#ifndef USE_WIN32
    (*section_ptr)->option.log_stderr=new_global_options.option.log_stderr;
#endif /* USE_WIN32 */

    if(*section_ptr==&new_service_options) {
        /* end of global options or inetd mode -> initialize globals */
        errstr=parse_global_option(CMD_INITIALIZE, NULL, NULL);
        if(errstr) {
            s_log(LOG_ERR, "Global options: %s", errstr);
            return 1;
        }
    }

    if(*section_ptr!=&new_service_options || eof) {
        /* end service section or inetd mode -> initialize service */
        if(*section_ptr==&new_service_options)
            s_log(LOG_INFO, "Initializing inetd mode configuration");
        else
            s_log(LOG_INFO, "Initializing service [%s]",
                (*section_ptr)->servname);
        errstr=parse_service_option(CMD_INITIALIZE, section_ptr, NULL, NULL);
        if(errstr) {
            if(*section_ptr==&new_service_options)
                s_log(LOG_ERR, "Inetd mode: %s", errstr);
            else
                s_log(LOG_ERR, "Service [%s]: %s",
                    (*section_ptr)->servname, errstr);
            return 1;
        }
    }
    return 0;
}