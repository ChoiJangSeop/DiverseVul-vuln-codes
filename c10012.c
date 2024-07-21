void options_defaults() {
    SERVICE_OPTIONS *service;

    /* initialize globals *before* opening the config file */
    memset(&new_global_options, 0, sizeof(GLOBAL_OPTIONS));
    memset(&new_service_options, 0, sizeof(SERVICE_OPTIONS));
    new_service_options.next=NULL;

    parse_global_option(CMD_SET_DEFAULTS, NULL, NULL);
    service=&new_service_options;
    parse_service_option(CMD_SET_DEFAULTS, &service, NULL, NULL);
}