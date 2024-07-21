void *auth_mellon_dir_merge(apr_pool_t *p, void *base, void *add)
{
    am_dir_cfg_rec *base_cfg = (am_dir_cfg_rec *)base;
    am_dir_cfg_rec *add_cfg = (am_dir_cfg_rec *)add;
    am_dir_cfg_rec *new_cfg;

    new_cfg = (am_dir_cfg_rec *)apr_palloc(p, sizeof(*new_cfg));

    apr_pool_cleanup_register(p, new_cfg, auth_mellon_free_server,
                              auth_mellon_free_server);


    new_cfg->enable_mellon = (add_cfg->enable_mellon != am_enable_default ?
                              add_cfg->enable_mellon :
                              base_cfg->enable_mellon);


    new_cfg->varname = (add_cfg->varname != default_cookie_name ?
                        add_cfg->varname :
                        base_cfg->varname);


    new_cfg->secure = (add_cfg->secure != default_secure_cookie ?
                        add_cfg->secure :
                        base_cfg->secure);

    new_cfg->merge_env_vars = (add_cfg->merge_env_vars != default_merge_env_vars ?
                               add_cfg->merge_env_vars :
                               base_cfg->merge_env_vars);

    new_cfg->env_vars_index_start = (add_cfg->env_vars_index_start != default_env_vars_index_start ?
                               add_cfg->env_vars_index_start :
                               base_cfg->env_vars_index_start);

    new_cfg->env_vars_count_in_n = (add_cfg->env_vars_count_in_n != default_env_vars_count_in_n ?
                               add_cfg->env_vars_count_in_n :
                               base_cfg->env_vars_count_in_n);

    new_cfg->cookie_domain = (add_cfg->cookie_domain != NULL ?
                        add_cfg->cookie_domain :
                        base_cfg->cookie_domain);

    new_cfg->cookie_path = (add_cfg->cookie_path != NULL ?
                        add_cfg->cookie_path :
                        base_cfg->cookie_path);

    new_cfg->cond = apr_array_copy(p,
                                   (!apr_is_empty_array(add_cfg->cond)) ?
                                   add_cfg->cond :
                                   base_cfg->cond);

    new_cfg->envattr = apr_hash_copy(p,
                                     (apr_hash_count(add_cfg->envattr) > 0) ?
                                     add_cfg->envattr :
                                     base_cfg->envattr);

    new_cfg->userattr = (add_cfg->userattr != default_user_attribute ?
                         add_cfg->userattr :
                         base_cfg->userattr);

    new_cfg->idpattr = (add_cfg->idpattr != NULL ?
                        add_cfg->idpattr :
                        base_cfg->idpattr);

    new_cfg->dump_session = (add_cfg->dump_session != default_dump_session ?
                             add_cfg->dump_session :
                             base_cfg->dump_session);

    new_cfg->dump_saml_response = 
        (add_cfg->dump_saml_response != default_dump_saml_response ?
         add_cfg->dump_saml_response :
         base_cfg->dump_saml_response);

    new_cfg->endpoint_path = (
        add_cfg->endpoint_path != default_endpoint_path ?
        add_cfg->endpoint_path :
        base_cfg->endpoint_path
        );

    new_cfg->session_length = (add_cfg->session_length != -1 ?
                               add_cfg->session_length :
                               base_cfg->session_length);

    new_cfg->no_cookie_error_page = (add_cfg->no_cookie_error_page != NULL ?
                                     add_cfg->no_cookie_error_page :
                                     base_cfg->no_cookie_error_page);

    new_cfg->no_success_error_page = (add_cfg->no_success_error_page != NULL ?
                                     add_cfg->no_success_error_page :
                                     base_cfg->no_success_error_page);


    new_cfg->sp_metadata_file = (add_cfg->sp_metadata_file ?
                                 add_cfg->sp_metadata_file :
                                 base_cfg->sp_metadata_file);

    new_cfg->sp_private_key_file = (add_cfg->sp_private_key_file ?
                                    add_cfg->sp_private_key_file :
                                    base_cfg->sp_private_key_file);

    new_cfg->sp_cert_file = (add_cfg->sp_cert_file ?
                             add_cfg->sp_cert_file :
                             base_cfg->sp_cert_file);

    new_cfg->idp_metadata = (add_cfg->idp_metadata->nelts ?
                             add_cfg->idp_metadata :
                             base_cfg->idp_metadata);

    new_cfg->idp_public_key_file = (add_cfg->idp_public_key_file ?
                                    add_cfg->idp_public_key_file :
                                    base_cfg->idp_public_key_file);

    new_cfg->idp_ca_file = (add_cfg->idp_ca_file ?
                            add_cfg->idp_ca_file :
                            base_cfg->idp_ca_file);

    new_cfg->idp_ignore = add_cfg->idp_ignore != NULL ?
                          add_cfg->idp_ignore :
                          base_cfg->idp_ignore;

    new_cfg->sp_entity_id = (add_cfg->sp_entity_id ?
                             add_cfg->sp_entity_id :
                             base_cfg->sp_entity_id);

    new_cfg->sp_org_name = apr_hash_copy(p,
                          (apr_hash_count(add_cfg->sp_org_name) > 0) ?
                           add_cfg->sp_org_name : 
                           base_cfg->sp_org_name);

    new_cfg->sp_org_display_name = apr_hash_copy(p,
                          (apr_hash_count(add_cfg->sp_org_display_name) > 0) ?
                           add_cfg->sp_org_display_name : 
                           base_cfg->sp_org_display_name);

    new_cfg->sp_org_url = apr_hash_copy(p,
                          (apr_hash_count(add_cfg->sp_org_url) > 0) ?
                           add_cfg->sp_org_url : 
                           base_cfg->sp_org_url);

    new_cfg->login_path = (add_cfg->login_path != default_login_path ?
                           add_cfg->login_path :
                           base_cfg->login_path);

    new_cfg->discovery_url = (add_cfg->discovery_url ?
                              add_cfg->discovery_url :
                              base_cfg->discovery_url);

    new_cfg->probe_discovery_timeout = 
                           (add_cfg->probe_discovery_timeout != -1 ?
                            add_cfg->probe_discovery_timeout :
                            base_cfg->probe_discovery_timeout);

    new_cfg->probe_discovery_idp = apr_table_copy(p,
                           (!apr_is_empty_table(add_cfg->probe_discovery_idp)) ?
                            add_cfg->probe_discovery_idp : 
                            base_cfg->probe_discovery_idp);


    if (cfg_can_inherit_lasso_server(add_cfg)) {
        new_cfg->inherit_server_from = base_cfg->inherit_server_from;
    } else {
        apr_thread_mutex_create(&new_cfg->server_mutex,
                                APR_THREAD_MUTEX_DEFAULT, p);
        new_cfg->inherit_server_from = new_cfg;
    }

    new_cfg->server = NULL;

    new_cfg->authn_context_class_ref = (add_cfg->authn_context_class_ref->nelts ?
                             add_cfg->authn_context_class_ref :
                             base_cfg->authn_context_class_ref);

    new_cfg->do_not_verify_logout_signature = apr_hash_copy(p, 
                             (apr_hash_count(add_cfg->do_not_verify_logout_signature) > 0) ?
                             add_cfg->do_not_verify_logout_signature :
                             base_cfg->do_not_verify_logout_signature);

    new_cfg->subject_confirmation_data_address_check =
        CFG_MERGE(add_cfg, base_cfg, subject_confirmation_data_address_check);
    new_cfg->post_replay = CFG_MERGE(add_cfg, base_cfg, post_replay);

    new_cfg->ecp_send_idplist = CFG_MERGE(add_cfg, base_cfg, ecp_send_idplist);

    return new_cfg;
}