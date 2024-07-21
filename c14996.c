static void kdc_post_fork(struct task_server *task, struct process_details *pd)
{
	struct kdc_server *kdc;
	krb5_kdc_configuration *kdc_config = NULL;
	NTSTATUS status;
	krb5_error_code ret;
	int ldb_ret;

	if (task == NULL) {
		task_server_terminate(task, "kdc: Null task", true);
		return;
	}
	if (task->private_data == NULL) {
		task_server_terminate(task, "kdc: No kdc_server info", true);
		return;
	}
	kdc = talloc_get_type_abort(task->private_data, struct kdc_server);

	/* get a samdb connection */
	kdc->samdb = samdb_connect(kdc,
				   kdc->task->event_ctx,
				   kdc->task->lp_ctx,
				   system_session(kdc->task->lp_ctx),
				   NULL,
				   0);
	if (!kdc->samdb) {
		DBG_WARNING("kdc_task_init: unable to connect to samdb\n");
		task_server_terminate(task, "kdc: krb5_init_context samdb connect failed", true);
		return;
	}

	ldb_ret = samdb_rodc(kdc->samdb, &kdc->am_rodc);
	if (ldb_ret != LDB_SUCCESS) {
		DBG_WARNING("kdc_task_init: "
			    "Cannot determine if we are an RODC: %s\n",
			    ldb_errstring(kdc->samdb));
		task_server_terminate(task, "kdc: krb5_init_context samdb RODC connect failed", true);
		return;
	}

	kdc->proxy_timeout = lpcfg_parm_int(kdc->task->lp_ctx, NULL, "kdc", "proxy timeout", 5);

	initialize_krb5_error_table();

	ret = smb_krb5_init_context(kdc, task->lp_ctx, &kdc->smb_krb5_context);
	if (ret) {
		DBG_WARNING("kdc_task_init: krb5_init_context failed (%s)\n",
			    error_message(ret));
		task_server_terminate(task, "kdc: krb5_init_context failed", true);
		return;
	}

	krb5_add_et_list(kdc->smb_krb5_context->krb5_context, initialize_hdb_error_table_r);

	ret = krb5_kdc_get_config(kdc->smb_krb5_context->krb5_context,
				  &kdc_config);
	if(ret) {
		task_server_terminate(task, "kdc: failed to get KDC configuration", true);
		return;
	}

	kdc_config->logf = (krb5_log_facility *)kdc->smb_krb5_context->pvt_log_data;
	kdc_config->db = talloc(kdc, struct HDB *);
	if (!kdc_config->db) {
		task_server_terminate(task, "kdc: out of memory", true);
		return;
	}
	kdc_config->num_db = 1;

	/*
	 * This restores the behavior before
	 * commit 255e3e18e00f717d99f3bc57c8a8895ff624f3c3
	 * s4:heimdal: import lorikeet-heimdal-201107150856
	 * (commit 48936803fae4a2fb362c79365d31f420c917b85b)
	 *
	 * as_use_strongest_session_key,preauth_use_strongest_session_key
	 * and tgs_use_strongest_session_key are input to the
	 * _kdc_find_etype() function. The old bahavior is in
	 * the use_strongest_session_key=FALSE code path.
	 * (The only remaining difference in _kdc_find_etype()
	 *  is the is_preauth parameter.)
	 *
	 * The old behavior in the _kdc_get_preferred_key()
	 * function is use_strongest_server_key=TRUE.
	 */
	kdc_config->tgt_use_strongest_session_key = false;
	kdc_config->preauth_use_strongest_session_key = true;
	kdc_config->svc_use_strongest_session_key = false;
	kdc_config->use_strongest_server_key = true;

	kdc_config->force_include_pa_etype_salt = true;

	/*
	 * For Samba CVE-2020-25719 Require PAC to be present
	 * This instructs Heimdal to match AD behaviour,
	 * as seen after Microsoft's CVE-2021-42287 when
	 * PacRequestorEnforcement is set to 2.
	 *
	 * Samba BUG: https://bugzilla.samba.org/show_bug.cgi?id=14686
	 * REF: https://support.microsoft.com/en-au/topic/kb5008380-authentication-updates-cve-2021-42287-9dafac11-e0d0-4cb8-959a-143bd0201041
	 */

	kdc_config->require_pac = true;

	/*
	 * By default we enable RFC6113/FAST support,
	 * but we have an option to disable in order to
	 * test against a KDC with FAST support.
	 */
	kdc_config->enable_fast = lpcfg_kdc_enable_fast(task->lp_ctx);

	/*
	 * Match Windows and RFC6113 and Windows but break older
	 * Heimdal clients.
	 */
	kdc_config->enable_armored_pa_enc_timestamp = false;

	/* Register hdb-samba4 hooks for use as a keytab */

	kdc->base_ctx = talloc_zero(kdc, struct samba_kdc_base_context);
	if (!kdc->base_ctx) {
		task_server_terminate(task, "kdc: out of memory", true);
		return;
	}

	kdc->base_ctx->ev_ctx = task->event_ctx;
	kdc->base_ctx->lp_ctx = task->lp_ctx;
	kdc->base_ctx->msg_ctx = task->msg_ctx;

	status = hdb_samba4_create_kdc(kdc->base_ctx,
				       kdc->smb_krb5_context->krb5_context,
				       &kdc_config->db[0]);
	if (!NT_STATUS_IS_OK(status)) {
		task_server_terminate(task, "kdc: hdb_samba4_create_kdc (setup KDC database) failed", true);
		return;
	}

	ret = krb5_plugin_register(kdc->smb_krb5_context->krb5_context,
				   PLUGIN_TYPE_DATA, "hdb_samba4_interface",
				   &hdb_samba4_interface);
	if(ret) {
		task_server_terminate(task, "kdc: failed to register hdb plugin", true);
		return;
	}

	kdc->keytab_name = talloc_asprintf(kdc, "HDBGET:samba4:&%p", kdc->base_ctx);
	if (kdc->keytab_name == NULL) {
		task_server_terminate(task,
				      "kdc: Failed to set keytab name",
				      true);
		return;
	}

	ret = krb5_kt_register(kdc->smb_krb5_context->krb5_context, &hdb_get_kt_ops);
	if(ret) {
		task_server_terminate(task, "kdc: failed to register keytab plugin", true);
		return;
	}

	/* Register KDC hooks */
	ret = krb5_plugin_register(kdc->smb_krb5_context->krb5_context,
				   PLUGIN_TYPE_DATA, "kdc",
				   &kdc_plugin_table);
	if(ret) {
		task_server_terminate(task, "kdc: failed to register kdc plugin", true);
		return;
	}

	ret = krb5_kdc_plugin_init(kdc->smb_krb5_context->krb5_context);

	if(ret) {
		task_server_terminate(task, "kdc: failed to init kdc plugin", true);
		return;
	}

	ret = krb5_kdc_pkinit_config(kdc->smb_krb5_context->krb5_context, kdc_config);

	if(ret) {
		task_server_terminate(task, "kdc: failed to init kdc pkinit subsystem", true);
		return;
	}
	kdc->private_data = kdc_config;

	status = IRPC_REGISTER(task->msg_ctx, irpc, KDC_CHECK_GENERIC_KERBEROS,
			       kdc_check_generic_kerberos, kdc);
	if (!NT_STATUS_IS_OK(status)) {
		task_server_terminate(task, "kdc failed to setup monitoring", true);
		return;
	}

	irpc_add_name(task->msg_ctx, "kdc_server");
}