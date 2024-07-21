init_usm(void)
{
    struct snmp_secmod_def *def;
    char *type;

    DEBUGMSGTL(("init_usm", "unit_usm: %" NETSNMP_PRIo "u %" NETSNMP_PRIo "u\n",
                usmNoPrivProtocol[0], usmNoPrivProtocol[1]));

    sc_init();                  /* initalize scapi code */

    /*
     * register ourselves as a security service
     */
    def = SNMP_MALLOC_STRUCT(snmp_secmod_def);
    if (def == NULL)
        return;
    /*
     * XXX: def->init_sess_secmod move stuff from snmp_api.c
     */
    def->encode_reverse = usm_secmod_rgenerate_out_msg;
    def->encode_forward = usm_secmod_generate_out_msg;
    def->decode = usm_secmod_process_in_msg;
    def->pdu_free_state_ref = usm_free_usmStateReference;
    def->session_setup = usm_session_init;
    def->handle_report = usm_handle_report;
    def->probe_engineid = usm_discover_engineid;
    def->post_probe_engineid = usm_create_user_from_session_hook;
    if (register_sec_mod(USM_SEC_MODEL_NUMBER, "usm", def) != SNMPERR_SUCCESS) {
        SNMP_FREE(def);
        snmp_log(LOG_ERR, "could not register usm sec mod\n");
        return;
    }

    snmp_register_callback(SNMP_CALLBACK_LIBRARY,
                           SNMP_CALLBACK_POST_PREMIB_READ_CONFIG,
                           init_usm_post_config, NULL);

    snmp_register_callback(SNMP_CALLBACK_LIBRARY,
                           SNMP_CALLBACK_SHUTDOWN,
                           deinit_usm_post_config, NULL);

    snmp_register_callback(SNMP_CALLBACK_LIBRARY,
                           SNMP_CALLBACK_SHUTDOWN,
                           free_engineID, NULL);

    register_config_handler("snmp", "defAuthType", snmpv3_authtype_conf,
                            NULL, "MD5|SHA|SHA-512|SHA-384|SHA-256|SHA-224");
    register_config_handler("snmp", "defPrivType", snmpv3_privtype_conf,
                            NULL,
                            "DES"
#ifdef HAVE_AES
                            "|AES|AES-128"
#ifdef NETSNMP_DRAFT_BLUMENTHAL_AES_04
                            "|AES-192|AES-256"
#endif /* NETSNMP_DRAFT_BLUMENTHAL_AES_04 */
#else
                            " (AES support not available)"
#endif
                           );

    /*
     * Free stuff at shutdown time
     */
    snmp_register_callback(SNMP_CALLBACK_LIBRARY,
                           SNMP_CALLBACK_SHUTDOWN,
                           free_enginetime_on_shutdown, NULL);


    type = netsnmp_ds_get_string(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_APPTYPE);

    register_config_handler(type, "userSetAuthPass", usm_set_password,
                            NULL, NULL);
    register_config_handler(type, "userSetPrivPass", usm_set_password,
                            NULL, NULL);
    register_config_handler(type, "userSetAuthKey", usm_set_password, NULL,
                            NULL);
    register_config_handler(type, "userSetPrivKey", usm_set_password, NULL,
                            NULL);
    register_config_handler(type, "userSetAuthLocalKey", usm_set_password,
                            NULL, NULL);
    register_config_handler(type, "userSetPrivLocalKey", usm_set_password,
                            NULL, NULL);
}