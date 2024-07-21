NOEXPORT int options_file(char *path, CONF_TYPE type,
        SERVICE_OPTIONS **section_ptr) {
    DISK_FILE *df;
    char line_text[CONFLINELEN], *errstr;
    char config_line[CONFLINELEN], *config_opt, *config_arg;
    int i, line_number=0;
#ifndef USE_WIN32
    int fd;
    char *tmp_str;
#endif

    s_log(LOG_NOTICE, "Reading configuration from %s %s",
        type==CONF_FD ? "descriptor" : "file", path);
#ifndef USE_WIN32
    if(type==CONF_FD) { /* file descriptor */
        fd=(int)strtol(path, &tmp_str, 10);
        if(tmp_str==path || *tmp_str) { /* not a number */
            s_log(LOG_ERR, "Invalid file descriptor number");
            print_syntax();
            return 1;
        }
        df=file_fdopen(fd);
    } else
#endif
        df=file_open(path, FILE_MODE_READ);
    if(!df) {
        s_log(LOG_ERR, "Cannot open configuration file");
        if(type!=CONF_RELOAD)
            print_syntax();
        return 1;
    }

    while(file_getline(df, line_text, CONFLINELEN)>=0) {
        memcpy(config_line, line_text, CONFLINELEN);
        ++line_number;
        config_opt=config_line;
        if(line_number==1) {
            if(config_opt[0]==(char)0xef &&
                    config_opt[1]==(char)0xbb &&
                    config_opt[2]==(char)0xbf) {
                s_log(LOG_NOTICE, "UTF-8 byte order mark detected");
                config_opt+=3;
            } else {
                s_log(LOG_NOTICE, "UTF-8 byte order mark not detected");
            }
        }

        while(isspace((unsigned char)*config_opt))
            ++config_opt; /* remove initial whitespaces */
        for(i=(int)strlen(config_opt)-1; i>=0 && isspace((unsigned char)config_opt[i]); --i)
            config_opt[i]='\0'; /* remove trailing whitespaces */
        if(config_opt[0]=='\0' || config_opt[0]=='#' || config_opt[0]==';') /* empty or comment */
            continue;

        if(config_opt[0]=='[' && config_opt[strlen(config_opt)-1]==']') { /* new section */
            if(init_section(0, section_ptr)) {
                file_close(df);
                return 1;
            }

            /* append a new SERVICE_OPTIONS structure to the list */
            {
                SERVICE_OPTIONS *new_section;
                new_section=str_alloc_detached(sizeof(SERVICE_OPTIONS));
                new_section->next=NULL;
                (*section_ptr)->next=new_section;
                *section_ptr=new_section;
            }

            /* initialize the newly allocated section */
            ++config_opt;
            config_opt[strlen(config_opt)-1]='\0';
            (*section_ptr)->servname=str_dup_detached(config_opt);
            (*section_ptr)->session=NULL;
            parse_service_option(CMD_SET_COPY, section_ptr, NULL, NULL);
            continue;
        }

        config_arg=strchr(config_line, '=');
        if(!config_arg) {
            s_log(LOG_ERR, "%s:%d: \"%s\": No '=' found",
                path, line_number, line_text);
            file_close(df);
            return 1;
        }
        *config_arg++='\0'; /* split into option name and argument value */
        for(i=(int)strlen(config_opt)-1; i>=0 && isspace((unsigned char)config_opt[i]); --i)
            config_opt[i]='\0'; /* remove trailing whitespaces */
        while(isspace((unsigned char)*config_arg))
            ++config_arg; /* remove initial whitespaces */

        errstr=option_not_found;
        /* try global options first (e.g. for 'debug') */
        if(!new_service_options.next)
            errstr=parse_global_option(CMD_SET_VALUE, config_opt, config_arg);
        if(errstr==option_not_found)
            errstr=parse_service_option(CMD_SET_VALUE, section_ptr, config_opt, config_arg);
        if(errstr) {
            s_log(LOG_ERR, "%s:%d: \"%s\": %s",
                path, line_number, line_text, errstr);
            file_close(df);
            return 1;
        }
    }
    file_close(df);
    return 0;
}