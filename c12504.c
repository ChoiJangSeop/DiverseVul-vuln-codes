char *parse_mount_options(ntfs_fuse_context_t *ctx,
			const struct ntfs_options *popts, BOOL low_fuse)
{
	char *options, *s, *opt, *val, *ret = NULL;
	const char *orig_opts = popts->options;
	BOOL no_def_opts = FALSE;
	int default_permissions = 0;
	int permissions = 0;
	int acl = 0;
	int want_permissions = 0;
	int intarg;
	const struct DEFOPTION *poptl;

	ctx->secure_flags = 0;
#ifdef HAVE_SETXATTR	/* extended attributes interface required */
	ctx->efs_raw = FALSE;
#endif /* HAVE_SETXATTR */
	ctx->compression = DEFAULT_COMPRESSION;
	options = strdup(orig_opts ? orig_opts : "");
	if (!options) {
		ntfs_log_perror("%s: strdup failed", EXEC_NAME);
		return NULL;
	}
	
	s = options;
	while (s && *s && (val = strsep(&s, ","))) {
		opt = strsep(&val, "=");
		poptl = optionlist;
		while (poptl->name && strcmp(poptl->name,opt))
			poptl++;
		if (poptl->name) {
			if ((poptl->flags & FLGOPT_BOGUS)
			    && bogus_option_value(val, opt))
				goto err_exit;
			if ((poptl->flags & FLGOPT_OCTAL)
			    && (!val
				|| !sscanf(val, "%o", &intarg))) {
				ntfs_log_error("'%s' option needs an octal value\n",
					opt);
				goto err_exit;
			}
			if (poptl->flags & FLGOPT_DECIMAL) {
				if ((poptl->flags & FLGOPT_OPTIONAL) && !val)
					intarg = 0;
				else
					if (!val
					    || !sscanf(val, "%i", &intarg)) {
						ntfs_log_error("'%s' option "
						     "needs a decimal value\n",
							opt);
						goto err_exit;
					}
			}
			if ((poptl->flags & FLGOPT_STRING)
			    && missing_option_value(val, opt))
				goto err_exit;

			switch (poptl->type) {
			case OPT_RO :
			case OPT_FAKE_RW :
				ctx->ro = TRUE;
				break;
			case OPT_RW :
				ctx->rw = TRUE;
				break;
			case OPT_NOATIME :
				ctx->atime = ATIME_DISABLED;
				break;
			case OPT_ATIME :
				ctx->atime = ATIME_ENABLED;
				break;
			case OPT_RELATIME :
				ctx->atime = ATIME_RELATIVE;
				break;
			case OPT_DMTIME :
				if (!intarg)
					intarg = DEFAULT_DMTIME;
				ctx->dmtime = intarg*10000000LL;
				break;
			case OPT_NO_DEF_OPTS :
				no_def_opts = TRUE; /* Don't add default options. */
				ctx->silent = FALSE; /* cancel default silent */
				break;
			case OPT_DEFAULT_PERMISSIONS :
				default_permissions = 1;
				break;
			case OPT_PERMISSIONS :
				permissions = 1;
				break;
#if POSIXACLS
			case OPT_ACL :
				acl = 1;
				break;
#endif
			case OPT_UMASK :
				ctx->dmask = ctx->fmask = intarg;
				want_permissions = 1;
				break;
			case OPT_FMASK :
				ctx->fmask = intarg;
			       	want_permissions = 1;
				break;
			case OPT_DMASK :
				ctx->dmask = intarg;
			       	want_permissions = 1;
				break;
			case OPT_UID :
				ctx->uid = intarg;
			       	want_permissions = 1;
				break;
			case OPT_GID :
				ctx->gid = intarg;
				want_permissions = 1;
				break;
			case OPT_SHOW_SYS_FILES :
				ctx->show_sys_files = TRUE;
				break;
			case OPT_HIDE_HID_FILES :
				ctx->hide_hid_files = TRUE;
				break;
			case OPT_HIDE_DOT_FILES :
				ctx->hide_dot_files = TRUE;
				break;
			case OPT_WINDOWS_NAMES :
				ctx->windows_names = TRUE;
				break;
			case OPT_IGNORE_CASE :
				if (low_fuse)
					ctx->ignore_case = TRUE;
				else {
					ntfs_log_error("'%s' is an unsupported option.\n",
						poptl->name);
					goto err_exit;
				}
				break;
			case OPT_COMPRESSION :
				ctx->compression = TRUE;
				break;
			case OPT_NOCOMPRESSION :
				ctx->compression = FALSE;
				break;
			case OPT_SILENT :
				ctx->silent = TRUE;
				break;
			case OPT_RECOVER :
				ctx->recover = TRUE;
				break;
			case OPT_NORECOVER :
				ctx->recover = FALSE;
				break;
			case OPT_REMOVE_HIBERFILE :
				ctx->hiberfile = TRUE;
				break;
			case OPT_SYNC :
				ctx->sync = TRUE;
				break;
#ifdef FUSE_CAP_BIG_WRITES
			case OPT_BIG_WRITES :
				ctx->big_writes = TRUE;
				break;
#endif
			case OPT_LOCALE :
				ntfs_set_char_encoding(val);
				break;
#if defined(__APPLE__) || defined(__DARWIN__)
#ifdef ENABLE_NFCONV
			case OPT_NFCONV :
				if (ntfs_macosx_normalize_filenames(1)) {
					ntfs_log_error("ntfs_macosx_normalize_filenames(1) failed!\n");
					goto err_exit;
				}
				break;
			case OPT_NONFCONV :
				if (ntfs_macosx_normalize_filenames(0)) {
					ntfs_log_error("ntfs_macosx_normalize_filenames(0) failed!\n");
					goto err_exit;
				}
				break;
#endif /* ENABLE_NFCONV */
#endif /* defined(__APPLE__) || defined(__DARWIN__) */
			case OPT_STREAMS_INTERFACE :
				if (!strcmp(val, "none"))
					ctx->streams = NF_STREAMS_INTERFACE_NONE;
				else if (!strcmp(val, "xattr"))
					ctx->streams = NF_STREAMS_INTERFACE_XATTR;
				else if (!strcmp(val, "openxattr"))
					ctx->streams = NF_STREAMS_INTERFACE_OPENXATTR;
				else if (!low_fuse && !strcmp(val, "windows"))
					ctx->streams = NF_STREAMS_INTERFACE_WINDOWS;
				else {
					ntfs_log_error("Invalid named data streams "
						"access interface.\n");
					goto err_exit;
				}
				break;
			case OPT_USER_XATTR :
#if defined(__APPLE__) || defined(__DARWIN__)
				/* macOS builds use non-namespaced extended
				 * attributes by default since it matches the
				 * standard behaviour of macOS filesystems. */
				ctx->streams = NF_STREAMS_INTERFACE_OPENXATTR;
#else
				ctx->streams = NF_STREAMS_INTERFACE_XATTR;
#endif
				break;
			case OPT_NOAUTO :
				/* Don't pass noauto option to fuse. */
				break;
			case OPT_DEBUG :
				ctx->debug = TRUE;
				ntfs_log_set_levels(NTFS_LOG_LEVEL_DEBUG);
				ntfs_log_set_levels(NTFS_LOG_LEVEL_TRACE);
				break;
			case OPT_NO_DETACH :
				ctx->no_detach = TRUE;
				break;
			case OPT_REMOUNT :
				ntfs_log_error("Remounting is not supported at present."
					" You have to umount volume and then "
					"mount it once again.\n");
				goto err_exit;
			case OPT_BLKSIZE :
				ntfs_log_info("WARNING: blksize option is ignored "
				      "because ntfs-3g must calculate it.\n");
				break;
			case OPT_INHERIT :
				/*
				 * do not overwrite inherited permissions
				 * in create()
				 */
				ctx->inherit = TRUE;
				break;
			case OPT_ADDSECURIDS :
				/*
				 * create security ids for files being read
				 * with an individual security attribute
				 */
				ctx->secure_flags |= (1 << SECURITY_ADDSECURIDS);
				break;
			case OPT_STATICGRPS :
				/*
				 * use static definition of groups
				 * for file access control
				 */
				ctx->secure_flags |= (1 << SECURITY_STATICGRPS);
				break;
			case OPT_USERMAPPING :
				ctx->usermap_path = strdup(val);
				if (!ctx->usermap_path) {
					ntfs_log_error("no more memory to store "
						"'usermapping' option.\n");
					goto err_exit;
				}
				break;
#ifdef HAVE_SETXATTR	/* extended attributes interface required */
#ifdef XATTR_MAPPINGS
			case OPT_XATTRMAPPING :
				ctx->xattrmap_path = strdup(val);
				if (!ctx->xattrmap_path) {
					ntfs_log_error("no more memory to store "
						"'xattrmapping' option.\n");
					goto err_exit;
				}
				break;
#endif /* XATTR_MAPPINGS */
			case OPT_EFS_RAW :
				ctx->efs_raw = TRUE;
				break;
#endif /* HAVE_SETXATTR */
			case OPT_POSIX_NLINK :
				ctx->posix_nlink = TRUE;
				break;
			case OPT_SPECIAL_FILES :
				if (!strcmp(val, "interix"))
					ctx->special_files = NTFS_FILES_INTERIX;
				else if (!strcmp(val, "wsl"))
					ctx->special_files = NTFS_FILES_WSL;
				else {
					ntfs_log_error("Invalid special_files"
						" mode.\n");
					goto err_exit;
				}
				break;
			case OPT_FSNAME : /* Filesystem name. */
			/*
			 * We need this to be able to check whether filesystem
			 * mounted or not.
			 *      (falling through to default)
			 */
			default :
				ntfs_log_error("'%s' is an unsupported option.\n",
					poptl->name);
				goto err_exit;
			}
			if ((poptl->flags & FLGOPT_APPEND)
			    && (ntfs_strappend(&ret, poptl->name)
				    || ntfs_strappend(&ret, ",")))
				goto err_exit;
		} else { /* Probably FUSE option. */
			if (ntfs_strappend(&ret, opt))
				goto err_exit;
			if (val) {
				if (ntfs_strappend(&ret, "="))
					goto err_exit;
				if (ntfs_strappend(&ret, val))
					goto err_exit;
			}
			if (ntfs_strappend(&ret, ","))
				goto err_exit;
		}
	}
	if (!no_def_opts && ntfs_strappend(&ret, def_opts))
		goto err_exit;
	if ((default_permissions || (permissions && !acl))
			&& ntfs_strappend(&ret, "default_permissions,"))
		goto err_exit;
			/* The atime options exclude each other */
	if (ctx->atime == ATIME_RELATIVE && ntfs_strappend(&ret, "relatime,"))
		goto err_exit;
	else if (ctx->atime == ATIME_ENABLED && ntfs_strappend(&ret, "atime,"))
		goto err_exit;
	else if (ctx->atime == ATIME_DISABLED && ntfs_strappend(&ret, "noatime,"))
		goto err_exit;
	
	if (ntfs_strappend(&ret, "fsname="))
		goto err_exit;
	if (ntfs_strappend(&ret, popts->device))
		goto err_exit;
	if (permissions && !acl)
		ctx->secure_flags |= (1 << SECURITY_DEFAULT);
	if (acl)
		ctx->secure_flags |= (1 << SECURITY_ACL);
	if (want_permissions)
		ctx->secure_flags |= (1 << SECURITY_WANTED);
	if (ctx->ro) {
		ctx->secure_flags &= ~(1 << SECURITY_ADDSECURIDS);
		ctx->hiberfile = FALSE;
		ctx->rw = FALSE;
	}
exit:
	free(options);
	return ret;
err_exit:
	free(ret);
	ret = NULL;
	goto exit;
}