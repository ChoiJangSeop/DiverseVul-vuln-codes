NTSTATUS mitkdc_task_init(struct task_server *task)
{
	struct tevent_req *subreq;
	const char * const *kdc_cmd;
	struct interface *ifaces;
	char *kdc_config = NULL;
	struct kdc_server *kdc;
	krb5_error_code code;
	NTSTATUS status;
	kadm5_ret_t ret;
	kadm5_config_params config;
	void *server_handle;
	int dbglvl = 0;

	task_server_set_title(task, "task[mitkdc_parent]");

	switch (lpcfg_server_role(task->lp_ctx)) {
	case ROLE_STANDALONE:
		task_server_terminate(task,
				      "The KDC is not required in standalone "
				      "server configuration, terminate!",
				      false);
		return NT_STATUS_INVALID_DOMAIN_ROLE;
	case ROLE_DOMAIN_MEMBER:
		task_server_terminate(task,
				      "The KDC is not required in member "
				      "server configuration",
				      false);
		return NT_STATUS_INVALID_DOMAIN_ROLE;
	case ROLE_ACTIVE_DIRECTORY_DC:
		/* Yes, we want to start the KDC */
		break;
	}

	/* Load interfaces for kpasswd */
	load_interface_list(task, task->lp_ctx, &ifaces);
	if (iface_list_count(ifaces) == 0) {
		task_server_terminate(task,
				      "KDC: no network interfaces configured",
				      false);
		return NT_STATUS_UNSUCCESSFUL;
	}

	kdc_config = talloc_asprintf(task,
				     "%s/kdc.conf",
				     lpcfg_private_dir(task->lp_ctx));
	if (kdc_config == NULL) {
		task_server_terminate(task,
				      "KDC: no memory",
				      false);
		return NT_STATUS_NO_MEMORY;
	}
	setenv("KRB5_KDC_PROFILE", kdc_config, 0);
	TALLOC_FREE(kdc_config);

	dbglvl = debuglevel_get_class(DBGC_KERBEROS);
	if (dbglvl >= 10) {
		char *kdc_trace_file = talloc_asprintf(task,
						       "%s/mit_kdc_trace.log",
						       get_dyn_LOGFILEBASE());
		if (kdc_trace_file == NULL) {
			task_server_terminate(task,
					"KDC: no memory",
					false);
			return NT_STATUS_NO_MEMORY;
		}

		setenv("KRB5_TRACE", kdc_trace_file, 1);
	}

	/* start it as a child process */
	kdc_cmd = lpcfg_mit_kdc_command(task->lp_ctx);

	subreq = samba_runcmd_send(task,
				   task->event_ctx,
				   timeval_zero(),
				   1, /* stdout log level */
				   0, /* stderr log level */
				   kdc_cmd,
				   "-n", /* Don't go into background */
#if 0
				   "-w 2", /* Start two workers */
#endif
				   NULL);
	if (subreq == NULL) {
		DEBUG(0, ("Failed to start MIT KDC as child daemon\n"));

		task_server_terminate(task,
				      "Failed to startup mitkdc task",
				      true);
		return NT_STATUS_INTERNAL_ERROR;
	}

	tevent_req_set_callback(subreq, mitkdc_server_done, task);

	DEBUG(5,("Started krb5kdc process\n"));

	status = samba_setup_mit_kdc_irpc(task);
	if (!NT_STATUS_IS_OK(status)) {
		task_server_terminate(task,
				      "Failed to setup kdc irpc service",
				      true);
	}

	DEBUG(5,("Started irpc service for kdc_server\n"));

	kdc = talloc_zero(task, struct kdc_server);
	if (kdc == NULL) {
		task_server_terminate(task, "KDC: Out of memory", true);
		return NT_STATUS_NO_MEMORY;
	}
	talloc_set_destructor(kdc, kdc_server_destroy);

	kdc->task = task;

	kdc->base_ctx = talloc_zero(kdc, struct samba_kdc_base_context);
	if (kdc->base_ctx == NULL) {
		task_server_terminate(task, "KDC: Out of memory", true);
		return NT_STATUS_NO_MEMORY;
	}

	kdc->base_ctx->ev_ctx = task->event_ctx;
	kdc->base_ctx->lp_ctx = task->lp_ctx;

	initialize_krb5_error_table();

	code = smb_krb5_init_context(kdc,
				     kdc->task->lp_ctx,
				     &kdc->smb_krb5_context);
	if (code != 0) {
		task_server_terminate(task,
				      "KDC: Unable to initialize krb5 context",
				      true);
		return NT_STATUS_INTERNAL_ERROR;
	}

	code = kadm5_init_krb5_context(&kdc->smb_krb5_context->krb5_context);
	if (code != 0) {
		task_server_terminate(task,
				      "KDC: Unable to init kadm5 krb5_context",
				      true);
		return NT_STATUS_INTERNAL_ERROR;
	}

	ZERO_STRUCT(config);
	config.mask = KADM5_CONFIG_REALM;
	config.realm = discard_const_p(char, lpcfg_realm(kdc->task->lp_ctx));

	ret = kadm5_init(kdc->smb_krb5_context->krb5_context,
			 discard_const_p(char, "kpasswd"),
			 NULL, /* pass */
			 discard_const_p(char, "kpasswd"),
			 &config,
			 KADM5_STRUCT_VERSION,
			 KADM5_API_VERSION_4,
			 NULL,
			 &server_handle);
	if (ret != 0) {
		task_server_terminate(task,
				      "KDC: Initialize kadm5",
				      true);
		return NT_STATUS_INTERNAL_ERROR;
	}
	kdc->private_data = server_handle;

	code = krb5_db_register_keytab(kdc->smb_krb5_context->krb5_context);
	if (code != 0) {
		task_server_terminate(task,
				      "KDC: Unable to KDB",
				      true);
		return NT_STATUS_INTERNAL_ERROR;
	}

	kdc->keytab_name = talloc_asprintf(kdc, "KDB:");
	if (kdc->keytab_name == NULL) {
		task_server_terminate(task,
				      "KDC: Out of memory",
				      true);
		return NT_STATUS_NO_MEMORY;
	}

	kdc->samdb = samdb_connect(kdc,
				   kdc->task->event_ctx,
				   kdc->task->lp_ctx,
				   system_session(kdc->task->lp_ctx),
				   NULL,
				   0);
	if (kdc->samdb == NULL) {
		task_server_terminate(task,
				      "KDC: Unable to connect to samdb",
				      true);
		return NT_STATUS_CONNECTION_INVALID;
	}

	status = startup_kpasswd_server(kdc,
				    kdc,
				    task->lp_ctx,
				    ifaces);
	if (!NT_STATUS_IS_OK(status)) {
		task_server_terminate(task,
				      "KDC: Unable to start kpasswd server",
				      true);
		return status;
	}

	DEBUG(5,("Started kpasswd service for kdc_server\n"));

	return NT_STATUS_OK;
}