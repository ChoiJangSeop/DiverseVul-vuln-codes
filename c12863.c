static void filter_parse_dyn_args(GF_Filter *filter, const char *args, GF_FilterArgType arg_type, Bool for_script, char *szSrc, char *szDst, char *szEscape, char *szSecName, Bool has_meta_args, u32 argfile_level)
{
	char *szArg=NULL;
	u32 i=0;
	u32 alloc_len=1024;
	const GF_FilterArgs *f_args = NULL;
	Bool opts_optional = GF_FALSE;

	if (args)
		szArg = gf_malloc(sizeof(char)*1024);

	//parse each arg
	while (args) {
		char *value;
		u32 len;
		Bool found=GF_FALSE;
		char *escaped = NULL;
		Bool opaque_arg = GF_FALSE;
		Bool absolute_url = GF_FALSE;
		Bool internal_url = GF_FALSE;
		Bool internal_arg = GF_FALSE;
		char *sep = NULL;

		//look for our arg separator - if arg[0] is also a separator, consider the entire string until next double sep as the parameter
		if (args[0] != filter->session->sep_args)
			sep = strchr(args, filter->session->sep_args);
		else {
			while (args[0] == filter->session->sep_args) {
				args++;
			}
			if (!args[0])
				break;

			sep = (char *) args + 1;
			while (1) {
				sep = strchr(sep, filter->session->sep_args);
				if (!sep) break;
				if (sep[1]==filter->session->sep_args) {
					break;
				}
				sep = sep+1;
			}
			opaque_arg = GF_TRUE;
		}

		if (!opaque_arg) {
			//we don't use gf_fs_path_escape_colon here because we also analyse whether the URL is internal or not, and we don't want to do that on each arg

			if (filter->session->sep_args == ':') {
				if (sep && !strncmp(args, szSrc, 4) && !strncmp(args+4, "gcryp://", 8)) {
					sep = strstr(args+12, "://");
				}
				while (sep && !strncmp(sep, "://", 3)) {
					absolute_url = GF_TRUE;

					//filter internal url schemes
					if ((!strncmp(args, szSrc, 4) || !strncmp(args, szDst, 4) ) &&
						(!strncmp(args+4, "video://", 8)
						|| !strncmp(args+4, "audio://", 8)
						|| !strncmp(args+4, "av://", 5)
						|| !strncmp(args+4, "gmem://", 7)
						|| !strncmp(args+4, "gpac://", 7)
						|| !strncmp(args+4, "pipe://", 7)
						|| !strncmp(args+4, "tcp://", 6)
						|| !strncmp(args+4, "udp://", 6)
						|| !strncmp(args+4, "tcpu://", 7)
						|| !strncmp(args+4, "udpu://", 7)
						|| !strncmp(args+4, "rtp://", 6)
						|| !strncmp(args+4, "atsc://", 7)
						|| !strncmp(args+4, "gfio://", 7)
						|| !strncmp(args+4, "route://", 8)
						)
					) {
						internal_url = GF_TRUE;
						sep = strchr(sep+3, ':');
						if (!strncmp(args+4, "tcp://", 6)
							|| !strncmp(args+4, "udp://", 6)
							|| !strncmp(args+4, "tcpu://", 7)
							|| !strncmp(args+4, "udpu://", 7)
							|| !strncmp(args+4, "rtp://", 6)
							|| !strncmp(args+4, "route://", 8)
						) {
							char *sep2 = sep ? strchr(sep+1, ':') : NULL;
							char *sep3 = sep ? strchr(sep+1, '/') : NULL;
							if (sep2 && sep3 && (sep2>sep3)) {
								sep2 = strchr(sep3, ':');
							}
							if (sep2 || sep3 || sep) {
								u32 port = 0;
								if (sep2) {
									sep2[0] = 0;
									if (sep3) sep3[0] = 0;
								}
								else if (sep3) sep3[0] = 0;
								if (sscanf(sep+1, "%d", &port)==1) {
									char szPort[20];
									snprintf(szPort, 20, "%d", port);
									if (strcmp(sep+1, szPort))
										port = 0;
								}
								if (sep2) sep2[0] = ':';
								if (sep3) sep3[0] = '/';

								if (port) sep = sep2;
							}
						}

					} else {
						//loog for '::' vs ':gfopt' and ':gpac:' - if '::' appears before these, jump to '::'
						char *sep2 = strstr(sep+3, ":gfopt:");
						char *sep3 = strstr(sep+3, szEscape);
						if (sep2 && sep3 && sep2<sep3)
							sep3 = sep2;
						else if (!sep3)
							sep3 = sep2;

						sep2 = strstr(sep+3, "::");
						if (sep2 && sep3 && sep3<sep2)
							sep2 = sep3;
						else if (sep2)
							opaque_arg = GF_TRUE; //skip an extra ':' at the end of the arg parsing

						//escape sequence present after this argument, use it
						if (sep2) {
							sep = sep2;
						} else {
							//get root /
							sep = strchr(sep+3, '/');
							//get first : after root
							if (sep) sep = strchr(sep+1, ':');
						}
					}
				}

				//watchout for "C:\\" or "C:/"
				while (sep && (sep[1]=='\\' || sep[1]=='/')) {
					sep = strchr(sep+1, ':');
				}
				//escape data/time
				if (sep) {
					char *prev_date = sep-3;
					if (prev_date[0]=='T') {}
					else if (prev_date[1]=='T') { prev_date ++; }
					else { prev_date = NULL; }

skip_date:
					if (prev_date) {
						u32 char_idx=1;
						u32 nb_date_seps=0;
						Bool last_non_num=GF_FALSE;
						char *search_after = NULL;
						while (1) {
							char dc = prev_date[char_idx];

							if ((dc>='0') && (dc<='9')) {
								search_after = prev_date+char_idx;
								char_idx++;
								continue;
							}
							if (dc == ':') {
								search_after = prev_date+char_idx;
								if (nb_date_seps>=3)
									break;
								char_idx++;
								nb_date_seps++;
								continue;
							}
							if ((dc == '.') || (dc == ';')) {
								if (last_non_num || !nb_date_seps) {
									search_after = NULL;
									break;
								}
								last_non_num = GF_TRUE;
								search_after = prev_date+char_idx;
								char_idx++;
								continue;
							}
							//not a valid char in date, stop
							break;
						}

						if (search_after) {
							//take care of lists
							char *next_date = strchr(search_after, 'T');
							char *next_sep = strchr(search_after, ':');
							if (next_date && next_sep && (next_date<next_sep)) {
								prev_date = next_date - 1;
								if (prev_date[0] == filter->session->sep_list) {
									prev_date = next_date;
									goto skip_date;
								}
							}
							sep = strchr(search_after, ':');
						}
					}
				}
			}
			if (sep) {
				escaped = strstr(sep, szEscape);
				if (escaped) sep = escaped;
			}

			if (sep && !strncmp(args, szSrc, 4) && !escaped && absolute_url && !internal_url) {
				Bool file_exists;
				sep[0]=0;
				if (!strcmp(args+4, "null")) file_exists = GF_TRUE;
				else if (!strncmp(args+4, "tcp://", 6)) file_exists = GF_TRUE;
				else if (!strncmp(args+4, "udp://", 6)) file_exists = GF_TRUE;
				else if (!strncmp(args+4, "route://", 8)) file_exists = GF_TRUE;
				else file_exists = gf_file_exists(args+4);

				if (!file_exists) {
					char *fsep = strchr(args+4, filter->session->sep_frag);
					if (fsep) {
						fsep[0] = 0;
						file_exists = gf_file_exists(args+4);
						fsep[0] = filter->session->sep_frag;
					}
				}
				sep[0]= filter->session->sep_args;
				if (!file_exists) {
					GF_LOG(GF_LOG_WARNING, GF_LOG_FILTER, ("Non-escaped argument pattern \"%s\" in src %s, assuming arguments are part of source URL. Use src=PATH:gpac:ARGS to differentiate, or change separators\n", sep, args));
					sep = NULL;
				}
			}
		}


		//escape some XML inputs
		if (sep) {
			char *xml_start = strchr(args, '<');
			len = (u32) (sep-args);
			if (xml_start) {
				u32 xlen = (u32) (xml_start-args);
				if ((xlen < len) && (args[len-1] != '>')) {
					while (1) {
						sep = strchr(sep+1, filter->session->sep_args);
						if (!sep) {
							len = (u32) strlen(args);
							break;
						}
						len = (u32) (sep-args);
						if (args[len-1] != '>') break;
					}
				}

			}
		}
		else len = (u32) strlen(args);

		if (len>=alloc_len) {
			alloc_len = len+1;
			szArg = gf_realloc(szArg, sizeof(char)*len);
		}
		strncpy(szArg, args, len);
		szArg[len]=0;

		value = strchr(szArg, filter->session->sep_name);
		if (value) {
			value[0] = 0;
			value++;
		}

		//arg is a PID property assignment
		if (szArg[0] == filter->session->sep_frag) {
			filter->user_pid_props = GF_TRUE;
			goto skip_arg;
		}

		if ((arg_type == GF_FILTER_ARG_INHERIT) && (!strcmp(szArg, "src") || !strcmp(szArg, "dst")) )
			goto skip_arg;

		i=0;
		f_args = filter->freg->args;
		if (for_script)
		 	f_args = filter->instance_args;

		while (filter->filter_udta && f_args) {
			Bool is_my_arg = GF_FALSE;
			Bool reverse_bool = GF_FALSE;
			const char *restricted=NULL;
			const GF_FilterArgs *a = &f_args[i];
			i++;
			if (!a || !a->arg_name) break;

			if (!strcmp(a->arg_name, szArg))
				is_my_arg = GF_TRUE;
			else if ( (szArg[0]==filter->session->sep_neg) && !strcmp(a->arg_name, szArg+1)) {
				is_my_arg = GF_TRUE;
				reverse_bool = GF_TRUE;
			}
			//little optim here, if no value check for enums
			else if (!value && a->min_max_enum && strchr(a->min_max_enum, '|') ) {
				char *arg_found = strstr(a->min_max_enum, szArg);
				if (arg_found) {
					char c = arg_found[strlen(szArg)];
					if (!c || (c=='|')) {
						is_my_arg = GF_TRUE;
						value = szArg;
					}
				}
			}

			if (is_my_arg) restricted = gf_opts_get_key_restricted(szSecName, a->arg_name);
			if (restricted) {
				GF_LOG(GF_LOG_WARNING, GF_LOG_FILTER, ("Argument %s of filter %s is restricted to %s by system-wide configuration, ignoring\n", szArg, filter->freg->name, restricted));
				found=GF_TRUE;
				break;
			}

			if (is_my_arg) {
				GF_PropertyValue argv;
				found=GF_TRUE;

				argv = gf_filter_parse_prop_solve_env_var(filter, (a->flags & GF_FS_ARG_META) ? GF_PROP_STRING : a->arg_type, a->arg_name, value, a->min_max_enum);

				if (reverse_bool && (argv.type==GF_PROP_BOOL))
					argv.value.boolean = !argv.value.boolean;

				if (argv.type != GF_PROP_FORBIDEN) {
					if (!for_script && (a->offset_in_private>=0)) {
						gf_filter_set_arg(filter, a, &argv);
					} else if (filter->freg->update_arg) {
						FSESS_CHECK_THREAD(filter)
						filter->freg->update_arg(filter, a->arg_name, &argv);
						opaque_arg = GF_FALSE;

						if ((argv.type==GF_PROP_STRING) && argv.value.string)
							gf_free(argv.value.string);
					}
				}
				break;
			}
		}
		if (!strlen(szArg)) {
			found = GF_TRUE;
		} else if (!found) {
			//filter ID
			if (!strcmp("FID", szArg)) {
				if (arg_type != GF_FILTER_ARG_INHERIT)
					gf_filter_set_id(filter, value);
				found = GF_TRUE;
				internal_arg = GF_TRUE;
			}
			//filter sources
			else if (!strcmp("SID", szArg)) {
				if (arg_type!=GF_FILTER_ARG_INHERIT)
					gf_filter_set_sources(filter, value);
				found = GF_TRUE;
				internal_arg = GF_TRUE;
			}
			//clonable filter
			else if (!strcmp("clone", szArg)) {
				if ((arg_type==GF_FILTER_ARG_EXPLICIT_SINK) || (arg_type==GF_FILTER_ARG_EXPLICIT))
					filter->clonable=GF_TRUE;
				found = GF_TRUE;
				internal_arg = GF_TRUE;
			}
			//filter name
			else if (!strcmp("N", szArg)) {
				if ((arg_type==GF_FILTER_ARG_EXPLICIT_SINK) || (arg_type==GF_FILTER_ARG_EXPLICIT) || (arg_type==GF_FILTER_ARG_EXPLICIT_SOURCE)) {
					gf_filter_set_name(filter, value);
				}
				found = GF_TRUE;
				internal_arg = GF_TRUE;
			}
			//internal options, nothing to do here
			else if (
				//generic encoder load
				!strcmp("c", szArg)
				//preferred registry to use
				|| !strcmp("gfreg", szArg)
				//non inherited options
				|| !strcmp("gfloc", szArg)
			) {
				found = GF_TRUE;
				internal_arg = GF_TRUE;
			}
			//non tracked options
			else if (!strcmp("gfopt", szArg)) {
				found = GF_TRUE;
				internal_arg = GF_TRUE;
				opts_optional = GF_TRUE;
			}
			//filter tag
			else if (!strcmp("TAG", szArg)) {
				if (! filter->dynamic_filter) {
					if (filter->tag) gf_free(filter->tag);
					filter->tag = value ? gf_strdup(value) : NULL;
				}
				found = GF_TRUE;
				internal_arg = GF_TRUE;
			}
			else if (gf_file_exists(szArg)) {
				if (!for_script && (argfile_level<5) ) {
					char szLine[2001];
					FILE *arg_file = gf_fopen(szArg, "rt");
					szLine[2000]=0;
					while (!gf_feof(arg_file)) {
						u32 llen;
						char *subarg, *res_line;
						szLine[0] = 0;
						res_line = gf_fgets(szLine, 2000, arg_file);
						if (!res_line) break;
						llen = (u32) strlen(szLine);
						while (llen && strchr(" \n\r\t", szLine[llen-1])) {
							szLine[llen-1]=0;
							llen--;
						}
						if (!llen)
							continue;

						subarg = szLine;
						while (subarg[0] && strchr(" \n\r\t", subarg[0]))
							subarg++;
						if ((subarg[0] == '/') && (subarg[1] == '/'))
							continue;

						filter_parse_dyn_args(filter, subarg, arg_type, for_script, szSrc, szDst, szEscape, szSecName, has_meta_args, argfile_level+1);
					}
					gf_fclose(arg_file);
				} else if (!for_script) {
					GF_LOG(GF_LOG_ERROR, GF_LOG_FILTER, ("Filter argument file has too many nested levels of sub-files, maximum allowed is 5\n"));
				}
				internal_arg = GF_TRUE;
			}
			else if (has_meta_args && filter->freg->update_arg) {
				GF_Err e = GF_OK;
				if (for_script || !(filter->freg->flags&GF_FS_REG_SCRIPT) ) {
					GF_PropertyValue argv = gf_props_parse_value(GF_PROP_STRING, szArg, value, NULL, filter->session->sep_list);
					FSESS_CHECK_THREAD(filter)
					e = filter->freg->update_arg(filter, szArg, &argv);
					if (argv.value.string) gf_free(argv.value.string);
				}
				if (!(filter->freg->flags&GF_FS_REG_SCRIPT) && (e==GF_OK) )
					found = GF_TRUE;
			}
		}

		if (!internal_arg && !opaque_arg && !opts_optional)
			gf_fs_push_arg(filter->session, szArg, found ? 1 : 0, 0);

skip_arg:
		if (escaped) {
			args=sep+6;
		} else if (sep) {
			args=sep+1;
			if (opaque_arg)
				args += 1;
		} else {
			args=NULL;
		}
	}
	if (szArg) gf_free(szArg);
}