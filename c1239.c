void *create_directory_config(apr_pool_t *mp, char *path)
{
    directory_config *dcfg = (directory_config *)apr_pcalloc(mp, sizeof(directory_config));
    if (dcfg == NULL) return NULL;

    #ifdef DEBUG_CONF
    ap_log_perror(APLOG_MARK, APLOG_STARTUP|APLOG_NOERRNO, 0, mp, "Created directory config %pp path %s", dcfg, path);
    #endif

    dcfg->mp = mp;
    dcfg->is_enabled = NOT_SET;

    dcfg->reqbody_access = NOT_SET;
    dcfg->reqintercept_oe = NOT_SET;
    dcfg->reqbody_buffering = NOT_SET;
    dcfg->reqbody_inmemory_limit = NOT_SET;
    dcfg->reqbody_limit = NOT_SET;
    dcfg->reqbody_no_files_limit = NOT_SET;
    dcfg->resbody_access = NOT_SET;

    dcfg->debuglog_name = NOT_SET_P;
    dcfg->debuglog_level = NOT_SET;
    dcfg->debuglog_fd = NOT_SET_P;

    dcfg->of_limit = NOT_SET;
    dcfg->if_limit_action = NOT_SET;
    dcfg->of_limit_action = NOT_SET;
    dcfg->of_mime_types = NOT_SET_P;
    dcfg->of_mime_types_cleared = NOT_SET;

    dcfg->cookie_format = NOT_SET;
    dcfg->argument_separator = NOT_SET;
    dcfg->cookiev0_separator = NOT_SET_P;

    dcfg->rule_inheritance = NOT_SET;
    dcfg->rule_exceptions = apr_array_make(mp, 16, sizeof(rule_exception *));
    dcfg->hash_method = apr_array_make(mp, 16, sizeof(hash_method *));

    /* audit log variables */
    dcfg->auditlog_flag = NOT_SET;
    dcfg->auditlog_type = NOT_SET;
    dcfg->max_rule_time = NOT_SET;
    dcfg->auditlog_dirperms = NOT_SET;
    dcfg->auditlog_fileperms = NOT_SET;
    dcfg->auditlog_name = NOT_SET_P;
    dcfg->auditlog2_name = NOT_SET_P;
    dcfg->auditlog_fd = NOT_SET_P;
    dcfg->auditlog2_fd = NOT_SET_P;
    dcfg->auditlog_storage_dir = NOT_SET_P;
    dcfg->auditlog_parts = NOT_SET_P;
    dcfg->auditlog_relevant_regex = NOT_SET_P;

    dcfg->ruleset = NULL;

    /* Upload */
    dcfg->tmp_dir = NOT_SET_P;
    dcfg->upload_dir = NOT_SET_P;
    dcfg->upload_keep_files = NOT_SET;
    dcfg->upload_validates_files = NOT_SET;
    dcfg->upload_filemode = NOT_SET;
    dcfg->upload_file_limit = NOT_SET;

    /* These are only used during the configuration process. */
    dcfg->tmp_chain_starter = NULL;
    dcfg->tmp_default_actionset = NULL;
    dcfg->tmp_rule_placeholders = NULL;

    /* Misc */
    dcfg->data_dir = NOT_SET_P;
    dcfg->webappid = NOT_SET_P;
    dcfg->sensor_id = NOT_SET_P;
    dcfg->httpBlkey = NOT_SET_P;

    /* Content injection. */
    dcfg->content_injection_enabled = NOT_SET;

    /* Stream inspection */
    dcfg->stream_inbody_inspection = NOT_SET;
    dcfg->stream_outbody_inspection = NOT_SET;

    /* Geo Lookups */
    dcfg->geo = NOT_SET_P;

    /* Gsb Lookups */
    dcfg->gsb = NOT_SET_P;

    /* Unicode Map */
    dcfg->u_map = NOT_SET_P;

    /* Cache */
    dcfg->cache_trans = NOT_SET;
    dcfg->cache_trans_incremental = NOT_SET;
    dcfg->cache_trans_min = NOT_SET;
    dcfg->cache_trans_max = NOT_SET;
    dcfg->cache_trans_maxitems = NOT_SET;

    /* Rule ids */
    dcfg->rule_id_htab = apr_hash_make(mp);
    dcfg->component_signatures = apr_array_make(mp, 16, sizeof(char *));

    dcfg->request_encoding = NOT_SET_P;
    dcfg->disable_backend_compression = NOT_SET;

    /* Collection timeout */
    dcfg->col_timeout = NOT_SET;

    dcfg->crypto_key = NOT_SET_P;
    dcfg->crypto_key_len = NOT_SET;
    dcfg->crypto_key_add = NOT_SET;
    dcfg->crypto_param_name = NOT_SET_P;
    dcfg->hash_is_enabled = NOT_SET;
    dcfg->hash_enforcement = NOT_SET;
    dcfg->crypto_hash_href_rx = NOT_SET;
    dcfg->crypto_hash_faction_rx = NOT_SET;
    dcfg->crypto_hash_location_rx = NOT_SET;
    dcfg->crypto_hash_iframesrc_rx = NOT_SET;
    dcfg->crypto_hash_framesrc_rx = NOT_SET;
    dcfg->crypto_hash_href_pm = NOT_SET;
    dcfg->crypto_hash_faction_pm = NOT_SET;
    dcfg->crypto_hash_location_pm = NOT_SET;
    dcfg->crypto_hash_iframesrc_pm = NOT_SET;
    dcfg->crypto_hash_framesrc_pm = NOT_SET;


    return dcfg;
}