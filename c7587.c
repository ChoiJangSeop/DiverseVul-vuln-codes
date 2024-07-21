void *auth_mellon_dir_config(apr_pool_t *p, char *d)
{
    am_dir_cfg_rec *dir = apr_palloc(p, sizeof(*dir));

    apr_pool_cleanup_register(p, dir, auth_mellon_free_server,
                              auth_mellon_free_server);

    dir->enable_mellon = am_enable_default;

    dir->varname = default_cookie_name;
    dir->secure = default_secure_cookie;
    dir->merge_env_vars = default_merge_env_vars;
    dir->env_vars_index_start = default_env_vars_index_start;
    dir->env_vars_count_in_n = default_env_vars_count_in_n;
    dir->cond = apr_array_make(p, 0, sizeof(am_cond_t));
    dir->cookie_domain = NULL;
    dir->cookie_path = NULL;
    dir->envattr   = apr_hash_make(p);
    dir->userattr  = default_user_attribute;
    dir->idpattr  = NULL;
    dir->dump_session = default_dump_session;
    dir->dump_saml_response = default_dump_saml_response;

    dir->endpoint_path = default_endpoint_path;

    dir->session_length = -1; /* -1 means use default. */

    dir->no_cookie_error_page = NULL;
    dir->no_success_error_page = NULL;

    dir->sp_metadata_file = NULL;
    dir->sp_private_key_file = NULL;
    dir->sp_cert_file = NULL;
    dir->idp_metadata = apr_array_make(p, 0, sizeof(am_metadata_t));
    dir->idp_public_key_file = NULL;
    dir->idp_ca_file = NULL;
    dir->idp_ignore = NULL;
    dir->login_path = default_login_path;
    dir->discovery_url = NULL;
    dir->probe_discovery_timeout = -1; /* -1 means no probe discovery */
    dir->probe_discovery_idp = apr_table_make(p, 0);

    dir->sp_entity_id = NULL;
    dir->sp_org_name = apr_hash_make(p);
    dir->sp_org_display_name = apr_hash_make(p);
    dir->sp_org_url = apr_hash_make(p);

    apr_thread_mutex_create(&dir->server_mutex, APR_THREAD_MUTEX_DEFAULT, p);
    dir->inherit_server_from = dir;
    dir->server = NULL;
    dir->authn_context_class_ref = apr_array_make(p, 0, sizeof(char *));
    dir->subject_confirmation_data_address_check = inherit_subject_confirmation_data_address_check;
    dir->do_not_verify_logout_signature = apr_hash_make(p);
    dir->post_replay = inherit_post_replay;

    dir->ecp_send_idplist = inherit_ecp_send_idplist;

    return dir;
}