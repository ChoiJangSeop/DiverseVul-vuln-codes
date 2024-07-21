int cli_matchmeta(cli_ctx *ctx, const char *fname, size_t fsizec, size_t fsizer, int encrypted, unsigned int filepos, int res1, void *res2)
{
	const struct cli_cdb *cdb;
	unsigned int viruses_found = 0;
        int ret = CL_CLEAN;

    cli_dbgmsg("CDBNAME:%s:%llu:%s:%llu:%llu:%d:%u:%u:%p\n",
	       cli_ftname(cli_get_container_type(ctx, -1)), (long long unsigned)fsizec, fname, (long long unsigned)fsizec, (long long unsigned)fsizer,
	       encrypted, filepos, res1, res2);

    if (ctx->engine && ctx->engine->cb_meta)
	if (ctx->engine->cb_meta(cli_ftname(cli_get_container_type(ctx, -1)), fsizec, fname, fsizer, encrypted, filepos, ctx->cb_ctx) == CL_VIRUS) {
	    cli_dbgmsg("inner file blacklisted by callback: %s\n", fname);

	    ret = cli_append_virus(ctx, "Detected.By.Callback");
	    viruses_found++;
	    if(!SCAN_ALL || ret != CL_CLEAN)
		return ret;
	}

    if(!ctx->engine || !(cdb = ctx->engine->cdb))
	return CL_CLEAN;

    do {
	if(cdb->ctype != CL_TYPE_ANY && cdb->ctype != cli_get_container_type(ctx, -1))
	    continue;

	if(cdb->encrypted != 2 && cdb->encrypted != encrypted)
	    continue;

	if(cdb->res1 && (cdb->ctype == CL_TYPE_ZIP || cdb->ctype == CL_TYPE_RAR) && cdb->res1 != res1)
	    continue;

#define CDBRANGE(field, val)						    \
	if(field[0] != CLI_OFF_ANY) {					    \
	    if(field[0] == field[1] && field[0] != val)			    \
		continue;						    \
	    else if(field[0] != field[1] && ((field[0] && field[0] > val) ||\
	      (field[1] && field[1] < val)))				    \
		continue;						    \
	}

	CDBRANGE(cdb->csize, cli_get_container_size(ctx, -1));
	CDBRANGE(cdb->fsizec, fsizec);
	CDBRANGE(cdb->fsizer, fsizer);
	CDBRANGE(cdb->filepos, filepos);

	if(cdb->name.re_magic && (!fname || cli_regexec(&cdb->name, fname, 0, NULL, 0) == REG_NOMATCH))
	    continue;

	ret = cli_append_virus(ctx, cdb->virname);
	viruses_found++;
	if(!SCAN_ALL || ret != CL_CLEAN)
	    return ret;

    } while((cdb = cdb->next));

    if (SCAN_ALL && viruses_found)
	return CL_VIRUS;
    return CL_CLEAN;
}