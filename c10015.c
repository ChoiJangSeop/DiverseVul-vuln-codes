int options_parse(CONF_TYPE type) {
    SERVICE_OPTIONS *section;

    options_defaults();
    section=&new_service_options;
    if(options_file(configuration_file, type, &section))
        return 1;
    if(init_section(1, &section))
        return 1;

    s_log(LOG_NOTICE, "Configuration successful");
    return 0;
}