GF_Filter *gf_fs_load_filter(GF_FilterSession *fsess, const char *name, GF_Err *err_code)
{
	const char *args=NULL;
	const char *sep, *file_ext;
	u32 i, len, count = gf_list_count(fsess->registry);
	Bool quiet = (err_code && (*err_code == GF_EOS)) ? GF_TRUE : GF_FALSE;

	assert(fsess);
	assert(name);
	if (err_code) *err_code = GF_OK;

	sep = gf_fs_path_escape_colon(fsess, name);
	if (sep) {
		args = sep+1;
		len = (u32) (sep - name);
	} else len = (u32) strlen(name);

	if (!len) {
		if (!quiet) {
			GF_LOG(GF_LOG_ERROR, GF_LOG_FILTER, ("Missing filter name in %s\n", name));
		}
		return NULL;
	}

	if (!strncmp(name, "enc", len)) {
		return gf_fs_load_encoder(fsess, args);
	}
	/*regular filter loading*/
	for (i=0;i<count;i++) {
		const GF_FilterRegister *f_reg = gf_list_get(fsess->registry, i);
		if ((strlen(f_reg->name)==len) && !strncmp(f_reg->name, name, len)) {
			GF_Filter *filter;
			GF_FilterArgType argtype = GF_FILTER_ARG_EXPLICIT;

			if ((f_reg->flags & GF_FS_REG_REQUIRES_RESOLVER) && !fsess->max_resolve_chain_len) {
				GF_LOG(GF_LOG_ERROR, GF_LOG_FILTER, ("Filter %s requires graph resolver but it is disabled\n", name));
				if (err_code) *err_code = GF_BAD_PARAM;
				return NULL;
			}

			if (f_reg->flags & GF_FS_REG_ACT_AS_SOURCE) argtype = GF_FILTER_ARG_EXPLICIT_SOURCE;
			filter = gf_filter_new(fsess, f_reg, args, NULL, argtype, err_code, NULL, GF_FALSE);
			if (!filter) return NULL;
			if (!filter->num_output_pids) {
				const char *src_url = strstr(name, "src");
				if (src_url && (src_url[3]==fsess->sep_name))
					gf_filter_post_process_task(filter);
			}
			return filter;
		}
	}
	/*check JS file*/
	file_ext = gf_file_ext_start(name);
	if (file_ext && (file_ext > sep) )
		file_ext = NULL;

	if (!file_ext || strstr(name, ".js") || strstr(name, ".jsf") || strstr(name, ".mjs") ) {
		Bool file_exists = GF_FALSE;
		char szName[10+GF_MAX_PATH];
		char szPath[10+GF_MAX_PATH];
		if (len>GF_MAX_PATH)
			return NULL;

		strncpy(szPath, name, len);
		szPath[len]=0;
		GF_LOG(GF_LOG_DEBUG, GF_LOG_FILTER, ("Trying JS filter %s\n", szPath));
		if (gf_file_exists(szPath)) {
			file_exists = GF_TRUE;
		} else {
			strcpy(szName, szPath);
			file_exists = gf_fs_solve_js_script(szPath, szName, file_ext);
			if (!file_exists && !file_ext) {
				strcat(szName, ".js");
				if (gf_file_exists(szName)) {
					strncpy(szPath, name, len);
					szPath[len]=0;
					strcat(szPath, ".js");
					file_exists = GF_TRUE;
				}
			}
		}

		if (file_exists) {
			sprintf(szName, "jsf%cjs%c", fsess->sep_args, fsess->sep_name);
			strcat(szName, szPath);
			if (name[len])
				strcat(szName, name+len);
			return gf_fs_load_filter(fsess, szName, err_code);
		}
	}

	if (!quiet) {
		GF_LOG(GF_LOG_ERROR, GF_LOG_FILTER, ("Failed to load filter %s: no such filter registry\n", name));
	}
	if (err_code) *err_code = GF_FILTER_NOT_FOUND;
	return NULL;
}