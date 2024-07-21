int parse_config(char *filename, bridge_t **bridges)
{
    dictionary *ubridge_config = NULL;
    const char *value;
    const char *bridge_name;
    int i, nsec;

    if ((ubridge_config = iniparser_load(filename)) == NULL) {
       return FALSE;
    }

    nsec = iniparser_getnsec(ubridge_config);
    for (i = 0; i < nsec; i++) {
        bridge_t *bridge;
        nio_t *source_nio = NULL;
        nio_t *destination_nio = NULL;

        bridge_name = iniparser_getsecname(ubridge_config, i);
        printf("Parsing %s\n", bridge_name);
        if (getstr(ubridge_config, bridge_name, "source_udp", &value))
           source_nio = create_udp_tunnel(value);
        else if (getstr(ubridge_config, bridge_name, "source_unix", &value))
           source_nio = create_unix_socket(value);
        else if (getstr(ubridge_config, bridge_name, "source_ethernet", &value))
           source_nio = open_ethernet_device(value);
        else if (getstr(ubridge_config, bridge_name, "source_tap", &value))
           source_nio = open_tap_device(value);
#ifdef LINUX_RAW
        else if (getstr(ubridge_config, bridge_name, "source_linux_raw", &value))
           source_nio = open_linux_raw(value);
#endif
#ifdef __APPLE__
        else if (getstr(ubridge_config, bridge_name, "source_fusion_vmnet", &value))
           source_nio = open_fusion_vmnet(value);
#endif
        else
           fprintf(stderr, "source NIO not found\n");

        if (getstr(ubridge_config, bridge_name, "destination_udp", &value))
           destination_nio = create_udp_tunnel(value);
        else if (getstr(ubridge_config, bridge_name, "destination_unix", &value))
           destination_nio = create_unix_socket(value);
        else if (getstr(ubridge_config, bridge_name, "destination_ethernet", &value))
           destination_nio = open_ethernet_device(value);
        else if (getstr(ubridge_config, bridge_name, "destination_tap", &value))
           destination_nio = open_tap_device(value);
#ifdef LINUX_RAW
        else if (getstr(ubridge_config, bridge_name, "destination_linux_raw", &value))
           source_nio = open_linux_raw(value);
#endif
#ifdef __APPLE__
        else if (getstr(ubridge_config, bridge_name, "destination_fusion_vmnet", &value))
           destination_nio = open_fusion_vmnet(value);
#endif
        else
           fprintf(stderr, "destination NIO not found\n");

        if (source_nio && destination_nio) {
           bridge = add_bridge(bridges);
           bridge->source_nio = source_nio;
           bridge->destination_nio = destination_nio;
           if (!(bridge->name = strdup(bridge_name))) {
              fprintf(stderr, "bridge creation: insufficient memory\n");
              return FALSE;
           }
           parse_capture(ubridge_config, bridge_name, bridge);
           parse_filter(ubridge_config, bridge_name, bridge);
        }
        else if (source_nio != NULL)
           free_nio(source_nio);
        else if (destination_nio != NULL)
           free_nio(destination_nio);
    }
    iniparser_freedict(ubridge_config);
    return TRUE;
}